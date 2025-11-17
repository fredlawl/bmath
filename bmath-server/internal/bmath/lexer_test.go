package bmath

import (
	"fmt"
	"slices"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestEnumerateTokens(t *testing.T) {
	content := `@test=2;
@test=1;
@hype=5;
popcnt(@hype)
whoops!
@
`

	lex, err := NewLexer()
	if !assert.NoError(t, err, "lexer allocated") {
		t.Fail()
	}
	defer lex.Close()

	iterator, iteratorErr := lex.EnumerateTokens(strings.NewReader(content))
	tokens := slices.Collect(iterator)
	if !assert.NoError(t, iteratorErr(), "tokens enumerating") {
		t.Fail()
	}

	assert.Greater(t, len(tokens), 0, "has tokens")
	for _, t := range tokens {
		fmt.Printf("%+v=%+v\n", t, t.Item)
	}
}

func TestEnumerateTokensForInput(t *testing.T) {
	// content := `\t\n  1\n`
	content := "\t\n    1\n"

	lex, err := NewLexer()
	if !assert.NoError(t, err, "lexer allocated") {
		t.Fail()
	}
	defer lex.Close()

	iterator, iteratorErr := lex.EnumerateTokens(strings.NewReader(content))
	tokens := slices.Collect(iterator)
	if !assert.NoError(t, iteratorErr(), "tokens enumerating") {
		t.Fail()
	}

	assert.Equal(t, 1, len(tokens), "has tokens")
	//for _, token := range tokens {
	//	t.Logf("%+v\n", token.Item)
	//}
}

func FuzzEnumerateTokens(f *testing.F) {
	f.Add("")
	f.Add("1\n")
	f.Add("@test=2;\n")
	f.Add("align(4, 5)\n")
	f.Add("whoops!\n")
	f.Add("\t\n    1\n")
	f.Add("@\n")

	f.Fuzz(func(t *testing.T, content string) {
		lex, err := NewLexer()
		if err != nil {
			t.Fatalf("lexer allocated: %v", err)
		}
		defer lex.Close()

		iterator, iteratorErr := lex.EnumerateTokens(strings.NewReader(content))
		tokens := slices.Collect(iterator)

		if err := iteratorErr(); err != nil {
			t.Fatalf("tokens enumerating: %v", err)
		}

		var prevLine uint
		for i, token := range tokens {
			if i > 0 && token.Item.Line < prevLine {
				t.Fatalf("token line number decreased: %d -> %d", prevLine, token.Item.Line)
			}

			prevLine = token.Item.Line
		}
	})
}

func TestEnumerateSymbols(t *testing.T) {
	content := "@test = 1;\nalign(4, 5)\n@test + 0777 - 1.9\n"

	lex, err := NewLexer()
	if !assert.NoError(t, err, "lexer allocated") {
		t.Fail()
	}
	defer lex.Close()

	tokIter, tokIteratorErr := lex.EnumerateTokens(strings.NewReader(content))
	tokens := slices.Collect(tokIter)
	if !assert.NoError(t, tokIteratorErr(), "tokens enumerating") {
		t.Fail()
	}

	assert.Equal(t, 16, len(tokens), "has tokens")
	//for _, token := range tokens {
	//	t.Logf("%+v\n", token.Item)
	//}

	iterator, iteratorErr := lex.EnumerateSymbols(strings.NewReader(content))
	symbols := slices.Collect(iterator)
	if !assert.NoError(t, iteratorErr(), "tokens enumerating") {
		t.Fail()
	}

	assert.Equal(t, 3, len(symbols), "has symbols")
	//for _, symbol := range symbols {
	//	t.Logf("%+v\n", symbol.Item)
	//}
}
