package bmath

import (
	"bufio"
	"errors"
	"io"
	"strings"
	"unicode"

	"fred.software/m/pkg/lsp"
)

type FormatSettings struct {
	TrimTrailingWhitespace bool
	TrimFinalNewlines      bool
	AddNewLineAtEndOfFile  bool
}

type Formatter interface {
	Format(r io.Reader) ([]lsp.TextEdit, error)
	FormatText(text *Text) (string, bool, error)
}

type BmathFormatter struct {
	settings *FormatSettings
	lexer    *Lexer
}

type Text struct {
	Text   string
	Length uint
	Number uint
	Offset uint
}

func TextFromString(s string) *Text {
	return &Text{
		Text:   s,
		Length: uint(len(s)),
		Offset: 0,
		Number: uint(0),
	}
}

func NewFormatter(lexer *Lexer, settings *FormatSettings) *BmathFormatter {
	return &BmathFormatter{
		settings: settings,
		lexer:    lexer,
	}
}

func (f *BmathFormatter) Format(r io.Reader) ([]lsp.TextEdit, error) {
	edits := make([]lsp.TextEdit, 0, 32)

	emptyLines := 0
	lineIndex := -1
	var lastNotEmptyLine *Text
	var firstEmptyLine *Text

	content := bufio.NewReader(r)
	for {
		text, ioErr := content.ReadString('\n')
		if ioErr != nil && !errors.Is(ioErr, io.EOF) || len(text) == 0 {
			break
		}

		lineIndex++
		line := Text{
			Text:   text,
			Length: uint(len(text)),
			Offset: 0,
			Number: uint(lineIndex),
		}

		// TODO: do something about handling errors
		edit, changed, err := f.FormatText(&line)

		if changed && err == nil {
			edits = append(edits, lsp.TextEdit{
				Range: lsp.Range{
					Start: lsp.Position{
						Line:      line.Number,
						Character: 0,
					},
					End: lsp.Position{
						Line:      line.Number,
						Character: line.Length,
					},
				},
				NewText: edit,
			})
		}

		// The trim accounts for all empty lines at the end and a line with all spaces
		if len(edit) == 1 && edit[0] == '\n' {
			if emptyLines == 0 {
				firstEmptyLine = &line
			}

			if errors.Is(ioErr, io.EOF) {
				break
			}

			emptyLines++
			continue
		}

		lastNotEmptyLine = &line

		if errors.Is(ioErr, io.EOF) {
			break
		}

		emptyLines = 0
	}

	if f.settings.TrimFinalNewlines {
		if lastNotEmptyLine != nil && lastNotEmptyLine.Length > 0 {
			// Remove newline
			if lastNotEmptyLine.Text[lastNotEmptyLine.Length-1] == '\n' && !f.settings.AddNewLineAtEndOfFile {
				edits = append(edits, lsp.TextEdit{
					Range: lsp.Range{
						Start: lsp.Position{
							Line:      lastNotEmptyLine.Number,
							Character: lastNotEmptyLine.Length - 1,
						},
						End: lsp.Position{
							Line:      lastNotEmptyLine.Number,
							Character: lastNotEmptyLine.Length - 1,
						},
					},
					NewText: "",
				})
			}
		}

		if firstEmptyLine != nil && emptyLines > 0 {
			edits = append(edits, lsp.TextEdit{
				Range: lsp.Range{
					Start: lsp.Position{
						Line:      firstEmptyLine.Number,
						Character: 0,
					},
					End: lsp.Position{
						Line:      firstEmptyLine.Number + uint(emptyLines),
						Character: 0,
					},
				},
				NewText: "",
			})
		}
	}

	// Add a line at the end
	if lastNotEmptyLine != nil &&
		lastNotEmptyLine.Length > 0 &&
		lastNotEmptyLine.Text[lastNotEmptyLine.Length-1] != '\n' &&
		f.settings.AddNewLineAtEndOfFile {
		edits = append(edits, lsp.TextEdit{
			Range: lsp.Range{
				Start: lsp.Position{
					Line:      lastNotEmptyLine.Number + 1,
					Character: 0,
				},
				End: lsp.Position{
					Line:      lastNotEmptyLine.Number + 1,
					Character: 0,
				},
			},
			NewText: "\n",
		})
	}

	return edits, nil
}

func (f *BmathFormatter) FormatText(text *Text) (string, bool, error) {
	var err error
	var result strings.Builder
	var whitespace strings.Builder
	changed := false

	if text.Length == 0 {
		return "", false, nil
	}

	if text.Length == 1 {
		return text.Text, false, nil
	}

	hasTrailingNewline := text.Text[text.Length-1] == '\n'
	textLengthAccountsNewline := text.Length - 1
	if hasTrailingNewline {
		textLengthAccountsNewline--
	}

	// strings.TrimRight(..., unicode.IsSpace)
	// Except we need the offset of left most non-whitespace
	// and whitespace itself for preservation if fmt option is set
	var idx uint = textLengthAccountsNewline
	for ; idx > 0; idx-- {
		if !unicode.IsSpace(rune(text.Text[idx])) {
			break
		}
	}

	whitespace.Write([]byte(text.Text[idx+1 : textLengthAccountsNewline+1]))

	// Rebuild text from tokens
	hasEdits := false
	iterator, iteratorErr := f.lexer.EnumerateTokens(strings.NewReader(text.Text))
	var token *Token
	var prevToken *Token
	for tokenResult := range iterator {
		if tokenResult.Dubious() {
			continue
		}

		prevToken = token
		token = tokenResult.Item

		tokenText := text.Text[token.Offset : token.Offset+token.Length]
		switch token.Type {
		case TokenTypeAssignment:
			fallthrough
		case TokenTypeOperation:
			fallthrough
		case TokenTypeFactorOperation:
			fallthrough
		case TokenTypeShiftOperation:
			fallthrough
		case TokenTypeAdditiveOp:
			_, err = result.WriteString(" " + tokenText + " ")
		case TokenTypeComma:
			_, err = result.WriteString(", ")
		case TokenTypeComment:
			if prevToken != nil {
				tokenText = text.Text[prevToken.Offset+prevToken.Length : token.Offset+token.Length]
			}

			if f.settings.TrimTrailingWhitespace {
				tokenText = strings.TrimRightFunc(tokenText, unicode.IsSpace)
			}

			_, err = result.WriteString(tokenText)
		default:
			_, err = result.WriteString(tokenText)
		}

		if err != nil {
			return text.Text, false, err
		}

		hasEdits = true
	}

	if iteratorErr() != nil {
		return text.Text, false, iteratorErr()
	}

	// to detect if line is invalid, and not all of it is trailing/leading whitespace
	if !hasEdits && uint(whitespace.Len()) != textLengthAccountsNewline {
		return text.Text, false, nil
	}

	if !f.settings.TrimTrailingWhitespace && result.Len() > 0 {
		result.WriteString(whitespace.String())
	}

	if hasTrailingNewline {
		result.WriteByte('\n')
	}

	// Length check in theory is quicker, default to direct comparison if length is same
	changed = result.Len() != int(text.Length) || result.String() != text.Text

	return result.String(), changed, nil
}
