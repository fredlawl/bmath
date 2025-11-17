package bmath

/*
#cgo LDFLAGS: -lbmath
#include <lexer.h>
#include <stdlib.h>
#include <stdio.h>
*/
import "C"

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"iter"
	"unsafe"

	"fred.software/m/pkg/util"
)

type Lexer struct {
	ptr         *C.struct_lexer
	settingsptr *C.struct_lexer_settings
}

func NewLexer() (*Lexer, error) {
	settingsptr := C.struct_lexer_settings{}

	ptr := C.lexer_new(&settingsptr)
	if ptr == nil {
		return nil, errors.New("bmath/lexer: unable to allocate lexer")
	}

	return &Lexer{
		ptr:         ptr,
		settingsptr: &settingsptr,
	}, nil
}

func (l *Lexer) Close() error {
	C.lexer_free(l.ptr)
	return nil
}

func (l *Lexer) EnumerateTokens(r io.Reader) (iter.Seq[util.IterResult[Token]], func() error) {
	var enumerateErr error

	finish := func() error {
		return enumerateErr
	}

	sequence := func(yield func(util.IterResult[Token]) bool) {
		lineCnt := uint(0)
		reader := bufio.NewReader(r)
		for {
			line, ioErr := reader.ReadString('\n')
			if ioErr != nil && !errors.Is(ioErr, io.EOF) {
				enumerateErr = fmt.Errorf("bmath/lexer: unable to enumerate tokens: %w", ioErr)
				return
			}

			linePtr := C.CString(line)
			defer C.free(unsafe.Pointer(linePtr))
			// Lexer itself just takes a pointer to a string, and to avoid
			// copying a document in its entirety into the lexer each time
			// it's best to buffer line-by line. This does mean line count
			// needs to be accounted here and not trust what the token
			// says because the lexer just knows it as a single line
			// each time
			C.lexer_init(l.ptr, linePtr, C.size_t(len(line)))

			for {
				ctok := C.lexer_next_token(l.ptr)
				result := util.IterResult[Token]{}

				tok := toToken(&ctok)
				if tok.Is(TokenTypeNull) {
					break
				}

				tokErr := C.lexer_errno(l.ptr)
				if tokErr != 0 && tokErr != C.EOF {
					result.Error = errors.New(C.GoString(C.lexer_error_str(l.ptr)))
				}

				// TODO: Hack around the fact the bmath lexer no longer deals with symbols
				if tok.Is(TokenTypeIdentifier) || tok.Is(TokenTypeVariable) {
					ident := C.GoString(C.lexer_token_ident(l.ptr, &ctok))
					if len(ident) == 0 {
						result.Error = errors.New("empty identifier")
					}

					tok.Symbol = &Symbol{
						Flags:      0,
						Identifier: ident,
						Type:       SymbolTypeNone,
						Token:      tok,
					}
				}

				// TODO: We need lookahead functionality to know if a symbol is defined
				// and if not, then report an error that the code is using and undefined
				// symbol. Basically need to to use the real bmath parser instead
				// of just tokenization and implement a parser in go.
				// At least using the real parser, we can get the evaluations
				// inline as well at some point with the inlay hints
				switch tok.Type {
				case TokenTypeVariable:
					tok.Symbol.Type = SymbolTypeVariable
				case TokenTypeIdentifier:
					// TODO: Rely on the fact functions are only built in
					_, ok := Documentation[tok.Symbol.Identifier]
					if ok {
						tok.Symbol.Type = SymbolTypeFunction
					} else {
						// TODO: another hack around symbols
						result.Error = errors.New("unknown identifier")
					}
				}

				tok.Line = lineCnt
				result.Item = tok

				if !yield(result) {
					return
				}
			}

			if errors.Is(ioErr, io.EOF) {
				return
			}

			lineCnt++
		}
	}

	return sequence, finish
}

func (l *Lexer) EnumerateSymbols(r io.Reader) (iter.Seq[util.IterResult[Symbol]], func() error) {
	iterator, iteratorErr := l.EnumerateTokens(r)
	finish := iteratorErr

	sequence := func(yield func(util.IterResult[Symbol]) bool) {
		for tokenResult := range iterator {
			if tokenResult.Dubious() {
				continue
			}

			sym, ok := tokenResult.Item.ToSymbol()
			if !ok || sym.Is(SymbolTypeNone) {
				continue
			}

			result := util.IterResult[Symbol]{
				Item:  sym,
				Error: tokenResult.Error,
			}

			if !yield(result) {
				return
			}
		}
	}

	return sequence, finish
}
