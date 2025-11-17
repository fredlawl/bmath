package main

import (
	"context"
	"flag"
	"fmt"
	"log/slog"
	"net"
	"os"
	"os/signal"
	"runtime"
	"runtime/pprof"
	"strconv"
	"syscall"
	"time"

	"fred.software/m/internal/bmath"
	"fred.software/m/internal/cmd"

	v2 "fred.software/m/pkg/jsonrpc/v2"
	"fred.software/m/pkg/lsp"
)

type ServerType int

const (
	ServerTypeStdin ServerType = iota
	ServerTypeUnix
	ServerTypeTCP
)

var (
	clientProcessID uint
	debug           bool
	log             cmd.FileFlag
	pipe            string
	playbackLog     cmd.FileFlag
	port            uint
	profileCPU      cmd.FileFlag
	profileMemory   cmd.FileFlag
)

func init() {
	// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#implementationConsiderations
	flag.UintVar(&clientProcessID, "clientProcessId", 0, "Optionally pass in callers PID for signal monitoring.\nThis will not be overwritten by client's incoming initialize request.\nThis is mostly meant for Unix Domain Socket/TCP uses, as a separate process from the client is likely to spin this up.")
	flag.BoolVar(&debug, "debug", false, "Enables debug mode.")
	flag.Var(&log, "log", "Path to server log, otherwise logs to stderr by default.\nCreates `FILE` if does not exist.")
	// TODO: just parse the IP address + port scheme, and make a new type to parse out whether its domain socket or ip address. Then the -port flag can be removed
	flag.StringVar(&pipe, "pipe", "", "Path to Unix Domain Socket or server's TCP IP address.")
	flag.Var(&playbackLog, "playback-log", "Records all requests into server for playback.\nUseful for recording various clients like neovim or vscode to debug the language server.\nCreates `FILE` if does not exist.")
	flag.UintVar(&port, "port", 0, "Port for the TCP IP address.\nRequired for the -pipe option with a TCP IP address.")
	flag.Var(&profileCPU, "profile-cpu", "Path to *.prof to run go tool pprof against.\nCreates `FILE` if does not exist.")
	flag.Var(&profileMemory, "profile-memory", "Path to *.mprof to run go tool pprof against.\nCreates `FILE` if does not exist.")
}

func run() error {
	var err error
	flag.Parse()

	// Assume stdin by default, unless pipe param is passed in
	serverType := ServerTypeStdin
	serverCancelContext, cancelFunction := context.WithCancel(context.Background())
	signals := make(chan os.Signal, 1)

	// TODO: Set a global timeout that if no init request comes in from client within a certain time, kill this
	if playbackLog.File != nil {
		defer cmd.SafeClose(playbackLog)
	}

	logWriter := os.Stderr
	if log.File != nil {
		logWriter = log.File
		defer cmd.SafeClose(log)
	}

	logLevel := slog.LevelInfo
	if debug {
		logLevel = slog.LevelDebug
	}

	logHandler := slog.NewTextHandler(logWriter, &slog.HandlerOptions{
		Level: logLevel,
	})

	slog.SetDefault(slog.New(logHandler))

	if profileCPU.File != nil {
		runtime.SetCPUProfileRate(500)
		pprof.StartCPUProfile(profileCPU.File)
		defer pprof.StopCPUProfile()
	}

	if profileMemory.File != nil {
		pprof.WriteHeapProfile(profileMemory.File)
		defer cmd.SafeClose(profileMemory)
	}

	signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		signal := <-signals
		slog.Warn(fmt.Sprintf("received signal: %v", signal))
		cancelFunction()
	}()

	if len(pipe) > 0 {
		_, err := os.Stat(pipe)
		if err != nil {
			if !os.IsNotExist(err) {
				return fmt.Errorf("failed to stat socket file: %s", pipe)
			}
			// Assume IP address at this point. Server binding would handle any errors from here
			serverType = ServerTypeTCP
		} else {
			serverType = ServerTypeUnix
		}
	}

	lex, err := bmath.NewLexer()
	if err != nil {
		return fmt.Errorf("unable to create language lexer: %w", err)
	}
	defer cmd.SafeClose(lex)

	server := lsp.NewServer(&lsp.ServerSettings{
		PlaybackLog: playbackLog.File,
	})
	defer cmd.SafeClose(server)

	ls := bmath.NewLanguageServer(lex)
	err = server.RegisterHandlers(ls.Handlers())
	if err != nil {
		return err
	}

	// notification
	err = server.RegisterHandler("$/cancelRequest", func(ctx context.Context, srv *lsp.Server, requestData *v2.Request) (any, error) {
		params, ok := requestData.Params.(map[string]any)
		if !ok {
			slog.Error("unable to parse params")
			return nil, nil
		}

		var id v2.RequestID
		idNumber, ok := params["id"].(float64)
		if !ok {
			idString, ok := params["id"].(string)
			if !ok {
				slog.Error("unable to parse params.id")
				return nil, nil
			}
			id = v2.RequestID(idString)
		} else {
			id = v2.RequestID(strconv.Itoa(int(idNumber)))
		}

		server.CancelRequest(id)
		return nil, nil
	})
	if err != nil {
		return err
	}

	// TODO: Figure out how to bake server state management into LSP server since it's not language server specific.
	err = server.RegisterHandler("shutdown", func(ctx context.Context, srv *lsp.Server, requestData *v2.Request) (any, error) {
		srv.Shutdown()
		return nil, nil
	})
	if err != nil {
		return err
	}

	err = server.RegisterHandler("exit", func(ctx context.Context, srv *lsp.Server, requestData *v2.Request) (any, error) {
		slog.Info("client sent exit...")
		cancelFunction()
		return nil, nil
	})
	if err != nil {
		return err
	}

	// TODO: Setup correct logging etc...
	err = server.RegisterHandler("$/setTrace", func(ctx context.Context, srv *lsp.Server, requestData *v2.Request) (any, error) {
		// notification
		// see: https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#setTrace
		return nil, nil
	})
	if err != nil {
		return err
	}

	if serverType == ServerTypeStdin {
		slog.Info("stdin connection")
		conn := lsp.NewStdinConn()
		server.Serve(serverCancelContext, conn)
		return nil
	}

	var listener net.Listener
	var network string

	listenConfig := net.ListenConfig{
		KeepAlive: 30 * time.Second,
	}

	switch serverType {
	case ServerTypeUnix:
		network = "unix"
	case ServerTypeTCP:
		network = "tcp"
		pipe = fmt.Sprintf("%s:%d", pipe, port)
	}

	listener, err = listenConfig.Listen(serverCancelContext, network, pipe)
	if err != nil {
		return fmt.Errorf("failed to establish listener: %w", err)
	}

	slog.Info(fmt.Sprintf("starting bmath language server on %s ... Press Ctrl+C to stop", listener.Addr().String()))
	go func() {
		for serverCancelContext.Err() == nil {
			peer, err := listener.Accept()
			if err != nil {
				slog.Warn(fmt.Sprintf("peer %s: error accepting peer: %v", peer.RemoteAddr().String(), err))
				continue
			}

			slog.Info(fmt.Sprintf("peer %s: accepted", peer.RemoteAddr().String()))
			server.Serve(serverCancelContext, peer)
		}
	}()

	<-serverCancelContext.Done()
	return nil
}

func main() {
	err := run()
	// TODO: This is a little weird, the log is configured from run() and
	// default logger is set from there. Really, main should parse params
	// pass the cfg into run()
	slog.Info("exiting...")
	if err != nil {
		slog.Error("exit in error", slog.Any("error", err))
		os.Exit(1)
	}
	os.Exit(0)
}
