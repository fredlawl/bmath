package bmath

import (
	"bufio"
	"errors"
	"io"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestBuffio(t *testing.T) {
	content := "testing\ntesting"
	r := bufio.NewReader(strings.NewReader(content))
	lines := make([]string, 0, 2)
	for {
		line, err := r.ReadString('\n')
		if err != nil && !errors.Is(err, io.EOF) {
			break
		}

		lines = append(lines, line)
		if errors.Is(err, io.EOF) {
			break
		}
	}

	assert.Equal(t, 2, len(lines), "two strings")
}

func TestFormatRemovesTrailingNewlines(t *testing.T) {
	lexer, err := NewLexer()
	if !assert.NoError(t, err, "no error") {
		t.FailNow()
	}

	content := "1234\n\n\n"
	formatSettings := FormatSettings{
		TrimFinalNewlines: true,
	}
	f := NewFormatter(lexer, &formatSettings)

	edits, err := f.Format(strings.NewReader(content))
	assert.NoError(t, err, "error")
	// TODO: Actually test that the change is made in the edits list
	assert.Greater(t, len(edits), 0, "multiple edits")
	// t.Fatalf("%+v\n", edits)
}

func TestFormatAddNewLineAtEndOfFile(t *testing.T) {
	content := "1234"
	lexer, err := NewLexer()

	if !assert.NoError(t, err, "no error") {
		t.FailNow()
	}

	formatSettings := FormatSettings{
		TrimFinalNewlines:     false,
		AddNewLineAtEndOfFile: true,
	}

	f := NewFormatter(lexer, &formatSettings)

	edits, err := f.Format(strings.NewReader(content))
	assert.NoError(t, err, "error")
	// TODO: Actually test that the change is made in the edits list
	assert.Equal(t, 1, len(edits), "one edit")
	// t.Fatalf("%+v\n", edits)
}

func TestFormatting(t *testing.T) {
	tests := []struct {
		name     string
		input    string
		expected string
		settings *FormatSettings
	}{
		{
			name:     "trim trailing whitespace",
			input:    "1234    \n",
			expected: "1234\n",
			settings: &FormatSettings{
				TrimTrailingWhitespace: true,
			},
		},
		{
			name:     "trim leading whitespace up to newline",
			input:    "    \n",
			expected: "\n",
			settings: &FormatSettings{},
		},
		{
			name:     "trim leading whitespace",
			input:    "     1234",
			expected: "1234",
			settings: &FormatSettings{},
		},
		{
			name:     "space between operators",
			input:    "1*2/3%4+5-6<<7>>8|9&10^11",
			expected: "1 * 2 / 3 % 4 + 5 - 6 << 7 >> 8 | 9 & 10 ^ 11",
			settings: &FormatSettings{},
		},
		{
			name:     "space after comma",
			input:    ",6",
			expected: ", 6",
			settings: &FormatSettings{},
		},
		{
			name:     "truncate space after comma",
			input:    "6 ,6",
			expected: "6, 6",
			settings: &FormatSettings{},
		},
		{
			name:     "keep trailing whitespace",
			input:    "6    ",
			expected: "6    ",
			settings: &FormatSettings{
				TrimTrailingWhitespace: false,
			},
		},
		{
			name:     "keep trailing whitespace newline",
			input:    "6    \n",
			expected: "6    \n",
			settings: &FormatSettings{
				TrimTrailingWhitespace: false,
			},
		},
		{
			name:     "whitespace ordering preserved",
			input:    "6\t\t \n",
			expected: "6\t\t \n",
			settings: &FormatSettings{
				TrimTrailingWhitespace: false,
			},
		},
		{
			name:     "prserve spacing for comments",
			input:    "mask(8)   // test",
			expected: "mask(8)   // test",
			settings: &FormatSettings{
				TrimTrailingWhitespace: false,
			},
		},
		{
			name:     "prserve spacing for comments, trim trailing",
			input:    "mask(8)   // test   ",
			expected: "mask(8)   // test",
			settings: &FormatSettings{
				TrimTrailingWhitespace: true,
			},
		},
	}

	lexer, err := NewLexer()
	if !assert.NoError(t, err, "no error") {
		t.FailNow()
	}

	for _, test := range tests {
		t.Run(test.name, func(tt *testing.T) {
			// Call the function under test
			f := NewFormatter(lexer, test.settings)
			actual, _, err := f.FormatText(TextFromString(test.input))
			if !assert.NoError(tt, err, "no error") {
				t.FailNow()
			}
			assert.Equal(tt, test.expected, actual, "string equals")
		})
	}
}
