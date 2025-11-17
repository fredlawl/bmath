package bmath

import (
	"bytes"
	"io"
	"testing"

	"fred.software/m/pkg/lsp"
	"github.com/stretchr/testify/assert"
)

func TestNewReaderFromPositionFuzzy(t *testing.T) {
	tests := []struct {
		name     string
		input    string
		expected string
		position *lsp.Position
	}{
		{
			name:     "empty",
			input:    "",
			expected: "",
			position: &lsp.Position{
				Line: 0,
			},
		},
		{
			name:     "extends past line",
			input:    "",
			expected: "",
			position: &lsp.Position{
				Line: 1,
			},
		},
		{
			name:     "single line",
			input:    "line1\n",
			expected: "line1\n",
			position: &lsp.Position{
				Line: 0,
			},
		},
		{
			name:     "single line no break",
			input:    "line1",
			expected: "line1",
			position: &lsp.Position{
				Line: 0,
			},
		},
		{
			name:     "returns first line",
			input:    "line1\nline2",
			expected: "line1\n",
			position: &lsp.Position{
				Line: 0,
			},
		},
		{
			name:     "returns second line",
			input:    "line1\nline2",
			expected: "line2",
			position: &lsp.Position{
				Line: 1,
			},
		},
		{
			name:     "in-between",
			input:    "line1\nline2",
			expected: "line1\n",
			position: &lsp.Position{
				Line:      0,
				Character: 2,
			},
		},
	}

	version := -1
	document := NewDocument("file://newReaderFromPosition", LanguageID("test"))
	for _, test := range tests {
		version++
		document.Commit(version, test.input)

		t.Run(test.name, func(tt *testing.T) {
			var buffer bytes.Buffer
			reader := document.NewReaderFromPositionFuzzy(test.position)
			_, err := io.Copy(&buffer, reader)
			if !assert.NoError(tt, err, "no error") {
				tt.Fail()
			}
			assert.Equal(tt, test.expected, buffer.String(), "string equals")
		})
	}
}

func TestNewReaderFromRangeFuzzy(t *testing.T) {
	tests := []struct {
		name     string
		input    string
		expected string
		position *lsp.Range
	}{
		{
			name:     "empty",
			input:    "",
			expected: "",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 0,
				},
			},
		},
		{
			name:     "extends past line",
			input:    "",
			expected: "",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 1,
				},
			},
		},
		{
			name:     "single line",
			input:    "line1\n",
			expected: "line1\n",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 0,
				},
			},
		},
		{
			name:     "single line no break",
			input:    "line1",
			expected: "line1",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 0,
				},
			},
		},
		{
			name:     "returns first line",
			input:    "line1\nline2",
			expected: "line1\n",
			position: &lsp.Range{
				Start: lsp.Position{},
				End: lsp.Position{
					Line: 0,
				},
			},
		},
		{
			name:     "returns second line",
			input:    "line1\nline2",
			expected: "line2",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 1,
				},
				End: lsp.Position{
					Line: 1,
				},
			},
		},
		{
			name:     "in-between",
			input:    "line1\nline2\nline3",
			expected: "line2\n",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 1,
				},
				End: lsp.Position{
					Line: 1,
				},
			},
		},
		{
			name:     "start > end",
			input:    "line1\nline2\nline3",
			expected: "line1\nline2\n",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 1,
				},
				End: lsp.Position{
					Line: 0,
				},
			},
		},
		{
			name:     "all",
			input:    "line1\nline2\nline3",
			expected: "line1\nline2\nline3",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 2,
				},
			},
		},
		{
			name:     "clamp end",
			input:    "line1\nline2\nline3",
			expected: "line1\nline2\nline3",
			position: &lsp.Range{
				Start: lsp.Position{
					Line: 0,
				},
				End: lsp.Position{
					Line: 4,
				},
			},
		},
	}

	version := -1
	document := NewDocument("file://newReaderFromPosition", LanguageID("test"))
	for _, test := range tests {
		version++
		document.Commit(version, test.input)

		t.Run(test.name, func(tt *testing.T) {
			var buffer bytes.Buffer
			reader := document.NewReaderFromRangeFuzzy(test.position)
			_, err := io.Copy(&buffer, reader)
			if !assert.NoError(tt, err, "no error") {
				tt.Fail()
			}
			assert.Equal(tt, test.expected, buffer.String(), "string equals")
		})
	}
}
