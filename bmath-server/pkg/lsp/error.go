package lsp

import v2 "fred.software/m/pkg/jsonrpc/v2"

const (
	ServerNotInitialized v2.ErrorCode = -32002
	UnknownError         v2.ErrorCode = -32001
	RequestFailed        v2.ErrorCode = -32803
	ServerCancelled      v2.ErrorCode = -32802
	ContentModified      v2.ErrorCode = -32801
	RequestCancelled     v2.ErrorCode = -32800
)

func init() {
	v2.RegisterErrorCode(ServerNotInitialized, "Server hasn't initialized")
}
