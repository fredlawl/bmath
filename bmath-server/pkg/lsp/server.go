package lsp

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"os"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"

	v2 "fred.software/m/pkg/jsonrpc/v2"
)

type (
	serverState int
	Server      struct {
		handlers            map[string]Handler
		sequence            atomic.Int32
		state               serverState
		cancelRequests      map[v2.RequestID]struct{}
		connectionWriteLock sync.Mutex
		cancelRequestsLock  sync.Mutex
		playbackLog         *os.File
	}
	serverRequest struct {
		sequence int
		request  *v2.Request
	}
	Handler func(ctx context.Context, server *Server, requestData *v2.Request) (any, error)
)

const (
	stateOpen serverState = iota
	stateInit
	stateShutdown
	stateClosed
)

type ServerSettings struct {
	PlaybackLog *os.File
}

func NewServer(settings *ServerSettings) *Server {
	handlers := make(map[string]Handler, 32)
	cancelRequests := make(map[v2.RequestID]struct{}, 32)
	return &Server{
		handlers:       handlers,
		state:          stateOpen,
		cancelRequests: cancelRequests,
		playbackLog:    settings.PlaybackLog,
	}
}

func (s *Server) CancelRequest(id v2.RequestID) {
	s.cancelRequestsLock.Lock()
	defer s.cancelRequestsLock.Unlock()
	s.cancelRequests[id] = struct{}{}
}

func (s *Server) Closed() bool {
	return s.state == stateClosed
}

func (s *Server) Shutdown() {
	s.state = stateShutdown
}

func (s *Server) Initialize() error {
	if s.state != stateOpen {
		return errors.New("lsp/server: cannot initialize server when it's shut down or closed")
	}

	s.state = stateInit
	return nil
}

func (s *Server) writePayload(conn io.ReadWriteCloser, payload any) {
	responseBytes, err := json.Marshal(payload)
	if err != nil {
		slog.Error("lsp/server: unable to encode response", slog.Any("error", err))
		return
	}

	s.connectionWriteLock.Lock()
	defer s.connectionWriteLock.Unlock()
	fmt.Fprintf(conn, "Content-Length: %d\r\n\r\n%s", len(responseBytes), responseBytes)
}

func (s *Server) nextRequest(ctx context.Context, r *bufio.Reader) (*serverRequest, error) {
	// headers := make(map[string]string)
	var contentLength int
	var body []byte

	for ctx.Err() == nil {
		line, err := r.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				return nil, err
			}

			return nil, fmt.Errorf("lsp/server: read up to next new line character: %w", err)
		}

		line = strings.TrimRight(line, "\r\n")
		if line == "" {
			break
		}

		k, v, ok := strings.Cut(line, ":")
		if !ok {
			return nil, fmt.Errorf("lsp/server: invalid header line: %q", line)
		}

		k = strings.TrimSpace(k)
		v = strings.TrimSpace(v)
		// headers[strings.ToLower(k)] = v

		if strings.EqualFold(k, "Content-Length") {
			n, err := strconv.Atoi(v)
			if err != nil || n < 0 {
				return nil, fmt.Errorf("lsp/server: invalid Content-Length: %q", v)
			}
			contentLength = n
		}
	}

	if contentLength < 0 {
		return nil, fmt.Errorf("lsp/server: missing Content-Length header")
	}

	body = make([]byte, contentLength)
	bytes, err := io.ReadFull(r, body)
	if err != nil {
		return nil, fmt.Errorf("lsp/server: unable to copy body content: %w", err)
	}

	if bytes <= 0 || ctx.Err() != nil {
		return nil, nil
	}

	if s.playbackLog != nil {
		// technically I shouldn't add a \n at here, but it makes reading playback nicer, and
		// the bmath server parses it just fine
		fmt.Fprintf(s.playbackLog, "Content-Length: %d\r\n\r\n%s\n", contentLength, body)
	}

	var request v2.Request
	err = json.Unmarshal(body, &request)
	if err != nil {
		return nil, fmt.Errorf("lsp/server: unable to unmarshal request body: %w: %q", err, body)
	}

	sequence := s.sequence.Add(1)

	slog.Debug("lsp/server: queued request", slog.Any("sequence", sequence), slog.Any("method", request.Method), slog.Any("request", body))
	return &serverRequest{
		sequence: int(sequence),
		request:  &request,
	}, nil
}

func (s *Server) tryCancelRequest(conn io.ReadWriteCloser, sequence int, requestId *v2.RequestID) bool {
	if requestId == nil {
		return false
	}

	s.cancelRequestsLock.Lock()
	defer s.cancelRequestsLock.Unlock()
	_, ok := s.cancelRequests[*requestId]
	if ok {
		delete(s.cancelRequests, *requestId)
		s.writePayload(conn, RequestCancelled.ToResponse(requestId, nil))
		slog.Debug("lsp/server: request cancelled", slog.Any("sequence", sequence))
		return true
	}

	return false
}

func (s *Server) handleRequest(ctx context.Context, req *serverRequest, conn io.ReadWriteCloser) {
	request := req.request
	notification := request.IsNotification()

	switch s.state {
	case stateClosed:
		return
	case stateOpen:
		if request.Method == "initialize" || request.Method == "exit" {
			break
		}

		if !notification {
			s.writePayload(conn, ServerNotInitialized.ToResponse(request.ID, nil))
		}
		return
	case stateShutdown:
		if request.Method == "exit" {
			break
		}

		if !notification {
			s.writePayload(conn, v2.InvalidRequest.ToResponse(request.ID, errors.New("server is shut down")))
		}
		return
	}

	// TODO: maybe not do this, need to have a server.Trace() function to call in handlers that will pull from context
	newCtx := context.WithValue(ctx, "conn", conn)
	handler, ok := s.handlers[request.Method]
	if !ok {
		custErr := fmt.Errorf("handler for method %q is not found", request.Method)
		if notification {
			slog.Error("lsp/server: unable to handle notification", slog.Any("sequence", req.sequence), slog.Any("error", custErr))
			return
		}

		s.writePayload(conn, v2.MethodNotFound.ToResponse(request.ID, custErr))
		return
	}

	// Before doing any processing, check if request should be cancelled before doing anything
	if s.tryCancelRequest(conn, req.sequence, request.ID) {
		return
	}

	responseData, err := handler(newCtx, s, request)
	if err != nil {
		slog.Error(fmt.Sprintf("lsp/server: handler for method %q failed", request.Method), slog.Any("sequence", req.sequence), slog.Any("error", err))
		s.writePayload(conn, RequestFailed.ToResponse(request.ID, err))
		return
	}

	// Check after processing to see if cancel request came in
	if s.tryCancelRequest(conn, req.sequence, request.ID) {
		return
	}

	if notification {
		slog.Debug("lsp/server: notification handled", slog.Any("sequence", req.sequence))
		return
	}

	rawResponseData, err := json.Marshal(responseData)
	if err != nil {
		slog.Error("lsp/server: unable to encode response", slog.Any("sequence", req.sequence), slog.Any("error", err))
		return
	}

	response := v2.Response{
		ID:      request.ID,
		Version: v2.RpcVersion(),
		Result:  rawResponseData,
		Error:   nil,
	}

	s.writePayload(conn, response)
	slog.Debug("lsp/server: request handled", slog.Any("sequence", req.sequence))
}

func (s *Server) Serve(ctx context.Context, conn io.ReadWriteCloser) {
	var wg sync.WaitGroup
	requestsQueue := 12
	// requestsPerWorker := 4
	// TODO: Create proper worker pool because if multiple connections come in,
	// the workers should be shared, not spawn N workers foreach connection. That'd
	// be nuts
	// TODO: Util I figure out how to have decent ordering, I can't have
	// multiple workers
	requestsPerWorker := requestsQueue
	numHandlerWorkers := max(requestsQueue/requestsPerWorker, 1)
	channel := make(chan *serverRequest, requestsQueue)

	go func() {
		r := bufio.NewReaderSize(conn, 1024)
		for {
			servRequest, err := s.nextRequest(ctx, r)
			if err != nil {
				if errors.Is(err, io.EOF) {
					break
				}

				// It's more accurate to say that a server can only have one session, but many clients to that session
				// Debatable if we kill the whole thing if unable to parse current request, it's a problem when requests
				// are chained though--like reading from stdin, the offsets could make parsing go out of order
				slog.Error("lsp/server: unable to parse request", slog.Any("error", err))
				break
			}

			if servRequest == nil {
				continue
			}

			channel <- servRequest
		}

		close(channel)
	}()

	// Handler workers
	for range numHandlerWorkers {
		wg.Add(1)
		go func(group *sync.WaitGroup) {
			defer group.Done()
			for !s.Closed() {
				select {
				case req, ok := <-channel:
					if !ok {
						return
					}

					s.handleRequest(ctx, req, conn)

				case <-ctx.Done():
					return
				}
			}
		}(&wg)
	}

	wg.Wait()
	conn.Close()
}

func (s *Server) Close() error {
	s.state = stateClosed
	return nil
}

func (s *Server) RegisterHandler(method string, handler Handler) error {
	_, ok := s.handlers[method]
	if ok {
		return errors.New("lsp/server: handler already registered")
	}

	s.handlers[method] = handler
	return nil
}

func (s *Server) RegisterHandlers(handlers map[string]Handler) error {
	for k, v := range handlers {
		err := s.RegisterHandler(k, v)
		if err != nil {
			return err
		}
	}
	return nil
}
