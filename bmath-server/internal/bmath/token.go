package bmath

/*
#include <token.h>
#include <symbol.h>
*/
import "C"

import (
	"unsafe"

	"fred.software/m/pkg/lsp"
)

//go:generate stringer -type=TokenType
type TokenType int

//go:generate stringer -type=SymbolType
type SymbolType int

const (
	TokenTypeNull TokenType = iota
	TokenTypeNumber
	TokenTypeOperation
	TokenTypeShiftOperation
	TokenTypeLeftParenthesis
	TokenTypeRightParanthesis
	TokenTypeBitwiseNot
	TokenTypeAdditiveOp
	TokenTypeFactorOperation
	TokenTypeComma
	TokenTypeAssignment
	TokenTypeIdentifier
	TokenTypeTerminator
	TokenTypeVariable
	TokenTypeComment
)

const (
	SymbolTypeNone SymbolType = iota
	SymbolTypeFunction
	SymbolTypeVariable
)

type Token struct {
	Attribute uint64
	Type      TokenType
	Line      uint
	Offset    uint
	Length    uint
	// TODO: Hack for now since bmath lexer doesn't do symbols anymore
	Symbol *Symbol
}

type Symbol struct {
	Flags      uint
	Type       SymbolType
	Identifier string
	Token      *Token
	Defined    bool
}

func toToken(tok *C.struct_token) *Token {
	attr := *(*C.uint64_t)(unsafe.Pointer(&tok.d[0]))
	return &Token{
		Attribute: uint64(attr),
		Type:      TokenType(tok._type),
		Line:      uint(tok.line),
		Offset:    uint(tok.offset),
		Length:    uint(tok.len),
	}
}

func (t *Token) Is(ttype TokenType) bool {
	return t.Type == ttype
}

func (t *Token) Range() *lsp.Range {
	return &lsp.Range{
		Start: lsp.Position{
			Line:      t.Line,
			Character: t.Offset,
		},
		End: lsp.Position{
			Line:      t.Line,
			Character: t.Offset + t.Length,
		},
	}
}

// TODO: This most certainly does not work anymore because lexer has nothing to do with symbols anymore
//func (t *Token) ToSymbol() (*Symbol, bool) {
//	if t.Type != TokenTypeIdentifier {
//		return nil, false
//	}
//
//	symAddress := uintptr(t.Attribute)
//	sym := (*C.struct_symbol)(unsafe.Pointer(symAddress))
//	identifier := C.GoString(C.symbol_ident(sym))
//
//	return &Symbol{
//		Flags:      uint(sym.flags),
//		Type:       SymbolType(sym._type),
//		Identifier: identifier,
//		Token:      t,
//	}, true
//}

func (t *Token) ToSymbol() (*Symbol, bool) {
	if t.Symbol == nil {
		return nil, false
	}

	return t.Symbol, true
}

func (t *Token) ContainsPosition(line uint, character uint) bool {
	return t.Line == line && t.ContainsCharacterOffset(character)
}

func (t *Token) ContainsCharacterOffset(character uint) bool {
	return character >= t.Offset && character < t.Offset+t.Length
}

func (s *Symbol) Is(stype SymbolType) bool {
	return s.Type == stype
}

func (s *Symbol) ToDocumentSymbol() *lsp.DocumentSymbol {
	var symbolKind lsp.SymbolKind
	switch s.Type {
	case SymbolTypeFunction:
		symbolKind = lsp.SymbolKindFunction
	case SymbolTypeVariable:
		symbolKind = lsp.SymbolKindVariable
	}

	return &lsp.DocumentSymbol{
		Name: s.Identifier,
		// Detail: util.Ptr("test"),
		Detail:         nil, // TODO: Add signature information for functions
		Kind:           symbolKind,
		Tags:           nil,
		Range:          *s.Token.Range(),
		SelectionRange: *s.Token.Range(),
	}
}
