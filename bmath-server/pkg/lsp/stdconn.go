package lsp

import (
	"errors"
	"log/slog"
	"os"
	"time"
)

type StdinConn struct {
	closed bool
}

func NewStdinConn() *StdinConn {
	return &StdinConn{
		closed: false,
	}
}

func (c *StdinConn) SetWriteDeadline(t time.Time) error {
	return nil
}

func (c *StdinConn) Read(p []byte) (n int, err error) {
	if c.closed {
		return 0, errors.New("stdcon: connection closed")
	}

	return os.Stdin.Read(p)
}

func (c *StdinConn) Write(p []byte) (n int, err error) {
	if c.closed {
		return 0, errors.New("stdcon: connection closed")
	}

	return os.Stdout.Write(p)
}

func (c *StdinConn) Close() error {
	slog.Info("conn closed...")
	c.closed = true
	os.Stdin.Close()
	return nil
}
