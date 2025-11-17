package cmd

import (
	"fmt"
	"io"
	"log/slog"
)

func SafeClose(closer io.Closer) {
	err := closer.Close()
	if err != nil {
		slog.Error(fmt.Errorf("unable to close handler: %w", err).Error())
	}
}
