package cmd

import (
	"fmt"
	"os"
)

type FileFlag struct {
	*os.File
}

func (f *FileFlag) Set(s string) error {
	file, err := os.Create(s)
	if err != nil {
		return fmt.Errorf("unable to create %q: %w", s, err)
	}
	f.File = file
	return nil
}

func (f *FileFlag) String() string {
	if f.File != nil {
		return f.Name()
	}
	return ""
}

func (f *FileFlag) Get() any {
	return f.File
}
