package bmath

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"strings"
	"sync"

	"fred.software/m/pkg/lsp"
	"fred.software/m/pkg/util"
)

type LanguageID string

const BmathLanguageID = LanguageID("bmath")

type Document struct {
	uri         string
	version     int
	languageID  LanguageID
	content     string // TODO: Consider mmap
	lineOffsets []uint
	numLines    uint
	lock        sync.Mutex
}

func NewDocument(uri string, languageID LanguageID) *Document {
	document := &Document{
		uri:        uri,
		languageID: languageID,
		version:    -1,
	}
	document.lineOffsets = make([]uint, 0)
	document.numLines = 0
	document.content = ""
	return document
}

func (d *Document) LanguageID() LanguageID {
	return d.languageID
}

func (d *Document) URI() string {
	return d.uri
}

func (d *Document) NewReader() io.Reader {
	d.lock.Lock()
	defer d.lock.Unlock()
	return strings.NewReader(d.content)
}

// NewReaderFromRangeFuzzy returns a reader starting at position through end
// of range. Disregards the character offset of start & end positions.
// This is done this way because it's more useful to operate on full lines.
func (d *Document) NewReaderFromRangeFuzzy(r *lsp.Range) io.Reader {
	d.lock.Lock()
	defer d.lock.Unlock()

	if r.End.Line < r.Start.Line {
		tmp := r.Start.Line
		r.Start.Line = r.End.Line
		r.End.Line = tmp
	}

	if d.numLines == 0 || r.Start.Line > d.numLines {
		return strings.NewReader("")
	}

	end := uint(len(d.content))
	if r.End.Line+1 < d.numLines {
		end = d.lineOffsets[r.End.Line+1]
	}

	return strings.NewReader(d.content[d.lineOffsets[r.Start.Line]:end])
}

// NewReadeFromPositionFuzzy returns a reader starting at position's line through end
// of line. Disregards the character offset of the position.
// It's more useful to operate on full lines.
func (d *Document) NewReaderFromPositionFuzzy(p *lsp.Position) io.Reader {
	d.lock.Lock()
	defer d.lock.Unlock()

	if d.numLines == 0 || p.Line > d.numLines {
		return strings.NewReader("")
	}

	end := uint(len(d.content))
	if p.Line+1 < d.numLines {
		end = d.lineOffsets[p.Line+1]
	}

	return strings.NewReader(d.content[d.lineOffsets[p.Line]:end])
}

func (d *Document) Commit(version int, content string) {
	d.lock.Lock()
	defer d.lock.Unlock()
	if version <= d.version {
		return
	}
	d.version = version
	d.content = content

	// LSP operates on lines instead of offsets as a whole. On commit
	// calculate line offsets at the expense of long writes. This allows
	// more efficient reads so that functions down the line do not need
	// to loop through all the lines every time if a Range is needed,
	// a slice can be copied to the reader instead of the exact
	// text to parse.
	d.numLines = 0
	offset := uint(0)
	lineOffsets := make([]uint, 0, 32)
	r := bufio.NewReader(strings.NewReader(content))
	for {
		text, ioErr := r.ReadString('\n')
		if ioErr != nil && !errors.Is(ioErr, io.EOF) || len(text) == 0 {
			break
		}

		lineOffsets = append(lineOffsets, offset)
		offset += uint(len(text))
		d.numLines++

		if errors.Is(ioErr, io.EOF) {
			break
		}
	}

	d.lineOffsets = lineOffsets
}

func (d *Document) semanticTokens(l *Lexer, r io.Reader) ([]uint, error) {
	data := make([]uint, 0, 32)

	deltaOffset := uint(0)
	deltaLine := uint(0)
	line := uint(0)
	offset := uint(0)

	iterator, iteratorErr := l.EnumerateTokens(r)
	for tokResult := range iterator {
		if tokResult.Dubious() {
			continue
		}

		tok := tokResult.Item
		semanticToken := tok.ToSemanticToken()
		if semanticToken == SemanticTokenNone {
			continue
		}

		deltaLine = tok.Line - line
		if deltaLine > 0 {
			line = tok.Line
			offset = 0
		}
		deltaOffset = uint(tok.Offset) - offset
		offset = uint(tok.Offset)

		// line, offset, length, token type (int), token modifiers (bitset)
		data = append(data, deltaLine, deltaOffset, uint(tok.Length), uint(semanticToken), 0)
	}

	if iteratorErr() != nil {
		return []uint{}, iteratorErr()
	}

	return data, nil
}

func (d *Document) SemanticTokensRange(l *Lexer, r *lsp.Range) ([]uint, error) {
	return d.semanticTokens(l, d.NewReaderFromRangeFuzzy(r))
}

func (d *Document) SemanticTokensFull(l *Lexer) ([]uint, error) {
	return d.semanticTokens(l, d.NewReader())
}

// Symbols returns a list of user-created symbols. Currently that's just variable
// assignments because bmath does not support user-defined functions.
func (d *Document) Symbols(l *Lexer) ([]*lsp.DocumentSymbol, error) {
	documentSymbols := make([]*lsp.DocumentSymbol, 0, 32)

	var prev *Token
	iterator, iteratorErr := l.EnumerateTokens(d.NewReader())
	for tokenResult := range iterator {
		if tokenResult.Dubious() {
			prev = nil
			continue
		}

		if tokenResult.Item.Is(TokenTypeAssignment) {
			if prev != nil && prev.Is(TokenTypeVariable) {
				sym, ok := prev.ToSymbol()
				if ok {
					documentSymbol := sym.ToDocumentSymbol()
					if documentSymbol != nil {
						documentSymbols = append(documentSymbols, documentSymbol)
					}
				}
			}
		}

		prev = tokenResult.Item
	}

	if iteratorErr() != nil {
		return []*lsp.DocumentSymbol{}, iteratorErr()
	}

	return documentSymbols, nil
}

// Symbols returns a symbol at the given position.
func (d *Document) SymbolAtPosition(l *Lexer, position *lsp.Position) (*Symbol, error) {
	var targetSymbol *Symbol
	symbolIterator, symbolIteratorErr := l.EnumerateSymbols(d.NewReaderFromPositionFuzzy(position))

	for symbolResult := range symbolIterator {
		symbol := symbolResult.Item
		if symbolResult.Dubious() {
			continue
		}

		if !symbol.Token.ContainsCharacterOffset(position.Character) {
			continue
		}

		targetSymbol = symbol
	}

	if symbolIteratorErr() != nil {
		return nil, fmt.Errorf("document: %v", symbolIteratorErr())
	}

	return targetSymbol, nil
}

func (d *Document) Format(f Formatter) ([]lsp.TextEdit, error) {
	return f.Format(d.NewReader())
}

func (d *Document) FormatRange(f Formatter, r *lsp.Range) ([]lsp.TextEdit, error) {
	return nil, errors.New("FormatRange not imlemented; TODO")
}

// TODO: Use an actual abstract syntax tree for this...
func (d *Document) InlayHints(l *Lexer, textRange *lsp.Range) ([]*lsp.InlayHint, error) {
	var tokenStack []*Token
	var argumentsStack []*Token
	var line uint = 0
	hints := make([]*lsp.InlayHint, 0, 32)

	push := func(s *[]*Token, t *Token) {
		*s = append(*s, t)
	}

	top := func(s *[]*Token) *Token {
		if len(*s) == 0 {
			return nil
		}
		return (*s)[len(*s)-1]
	}

	pop := func(s *[]*Token) *Token {
		if len(*s) == 0 {
			return nil
		}
		t := (*s)[len(*s)-1]
		*s = (*s)[:len(*s)-1]
		return t
	}

	clr := func(s *[]*Token) {
		*s = (*s)[:0]
	}

	addHint := func(function *Symbol, arguments []*Token) {
		signature, ok := Documentation[function.Identifier]
		if !ok {
			return
		}

		params := signature.Parameters
		if params == nil || len(*params) == 0 || len(arguments) == 0 || len(*params) != len(arguments) {
			return
		}

		for i := len(arguments) - 1; i >= 0; i-- {
			// parameters are in order, arguments stack is reversed
			argument := arguments[len(arguments)-1-i]
			param := (*params)[i]
			hint := &lsp.InlayHint{
				Position: lsp.Position{
					Line:      argument.Line,
					Character: uint(argument.Offset) + 1,
				},
				Label:        param.Label + ":",
				Kind:         util.Ptr(lsp.InlayHintKindParameter),
				PaddingRight: util.Ptr(i == 0), // TODO: OR next character is not a space
				PaddingLeft:  util.Ptr(i > 0),
				Tooltip:      util.Ptr(param.Documentation),
			}
			hints = append(hints, hint)
		}
	}

	content := d.NewReaderFromRangeFuzzy(textRange)
	iterator, iteratorErr := l.EnumerateTokens(content)
	for tokenResult := range iterator {
		if tokenResult.Dubious() {
			continue
		}

		token := tokenResult.Item

		// ensure parsing line-by-line so that currnt line parsing errors don't cascade
		if token.Line != line {
			line = token.Line
			clr(&tokenStack)
		}

		// Essentially reverse-polish parsing algorithm
		if token.Is(TokenTypeRightParanthesis) {
			for {
				pt := pop(&tokenStack)
				if pt == nil {
					break
				}

				if pt.Is(TokenTypeComma) {
					push(&argumentsStack, pt)
				}

				if pt.Is(TokenTypeLeftParenthesis) {
					maybeFunction := top(&tokenStack)
					if maybeFunction == nil {
						break
					}

					symbol, ok := maybeFunction.ToSymbol()
					if !ok || !symbol.Is(SymbolTypeFunction) {
						break
					}

					push(&argumentsStack, pt)
					addHint(symbol, argumentsStack)
					clr(&argumentsStack)
					break
				}
			}

			continue
		}

		push(&tokenStack, token)
	}

	if iteratorErr() != nil {
		return nil, iteratorErr()
	}

	return hints, nil
}
