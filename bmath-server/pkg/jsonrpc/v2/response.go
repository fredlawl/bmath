package v2

import (
	"encoding/json"
	"errors"
)

type ErrorCode int

const (
	ParseError     ErrorCode = -32700
	InvalidRequest ErrorCode = -32600
	MethodNotFound ErrorCode = -32601
	InvalidParams  ErrorCode = -32602
	InternalError  ErrorCode = -32603
)

var errorMessages map[ErrorCode]string

func init() {
	errorMessages = map[ErrorCode]string{
		ParseError:     "Parse error",
		InvalidRequest: "Invalid request",
		MethodNotFound: "Method not found",
		InvalidParams:  "Invalid params",
		InternalError:  "Internal error",
	}
}

type Error struct {
	Code    ErrorCode       `json:"code"`
	Message string          `json:"message"`
	Data    json.RawMessage `json:"data,omitempty"`
}

type Response struct {
	ID      *RequestID      `json:"id"`
	Version Version         `json:"jsonrpc"`
	Result  json.RawMessage `json:"result,omitempty"`
	Error   *Error          `json:"error,omitempty"`
}

func (ec ErrorCode) Error() string {
	message, ok := errorMessages[ec]
	if !ok {
		return errorMessages[InternalError]
	}

	return message
}

func (ec ErrorCode) ToResponse(requestID *RequestID, err error) *Response {
	resp := &Response{
		ID:      requestID,
		Version: RpcVersion(),
		Error: &Error{
			Code:    ec,
			Message: ec.Error(),
			Data:    json.RawMessage(""),
		},
	}

	if err == nil {
		return resp
	}

	resp.Error.Data, err = json.Marshal(err.Error())
	if err != nil {
		resp.Error.Data = json.RawMessage("")
	}

	return resp
}

// RegisterErrorCode registers custom error messages to custom
// implementation error codes. An error may be rendered if the
// code attempts to overwrite a previously registered
// messages--including package registered messages, or if the code is
// out of spec range.
func RegisterErrorCode(code ErrorCode, message string) error {
	if code < -32099 || code > -32000 {
		return errors.New("jsonrpc/v2: error code is out of range [-32000, -32099]")
	}

	_, ok := errorMessages[code]
	if ok {
		return errors.New("jsonrpc/v2: error message for code is already registered")
	}
	errorMessages[code] = message
	return nil
}
