package bmath

type SemanticToken uint

const (
	SemanticTokenFunction SemanticToken = iota
	SemanticTokenVariable
	SemanticTokenKeyword
	SemanticTokenNumber
	SemanticTokenOperator
	SemanticTokenComment
	SemanticTokenNone = 999
)

var SemanticTokens = []string{"function", "variable", "keyword", "number", "operator", "comment"}

func (t *Token) ToSemanticToken() SemanticToken {
	switch t.Type {
	case TokenTypeIdentifier:
		sym, ok := t.ToSymbol()
		if !ok {
			return SemanticTokenNone
		}

		switch sym.Type {
		case SymbolTypeFunction:
			return SemanticTokenFunction
		case SymbolTypeVariable:
			return SemanticTokenVariable
		}
	case TokenTypeOperation:
		fallthrough
	case TokenTypeFactorOperation:
		fallthrough
	case TokenTypeAdditiveOp:
		fallthrough
	case TokenTypeAssignment:
		fallthrough
	case TokenTypeShiftOperation:
		return SemanticTokenOperator
	case TokenTypeNumber:
		return SemanticTokenNumber
	case TokenTypeComment:
		return SemanticTokenComment
	}

	return SemanticTokenNone
}
