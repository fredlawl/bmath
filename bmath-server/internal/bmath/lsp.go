package bmath

import (
	"context"
	"errors"
	"fmt"

	v2 "fred.software/m/pkg/jsonrpc/v2"
	"fred.software/m/pkg/lsp"
	"fred.software/m/pkg/util"
)

type (
	BmathLanguageServer struct {
		// TODO: Needs to have some locking around this
		lexer     *Lexer
		documents map[string]*Document
	}
)

func NewLanguageServer(lexer *Lexer) *BmathLanguageServer {
	return &BmathLanguageServer{
		documents: make(map[string]*Document, 32),
		lexer:     lexer,
	}
}

func (ls *BmathLanguageServer) Handlers() map[string]lsp.Handler {
	return map[string]lsp.Handler{
		"initialize":                       ls.Initialize,
		"initialized":                      ls.Initialized,
		"textDocument/didOpen":             ls.TextDocumentDidOpen,
		"textDocument/didClose":            ls.TextDocumentDidClose,
		"textDocument/didChange":           ls.TextDocumentDidChange,
		"textDocument/semanticTokens/full": ls.TextDocumentSemanticTokensFull,
		//"textDocument/semanticTokens/full/delta": ls.TextDocumentSemanticTokensFullDelta,
		"textDocument/semanticTokens/range": ls.TextDocumentSemanticTokensRange,
		"textDocument/documentSymbol":       ls.TextDocumentDocumentSymbol,
		"textDocument/signatureHelp":        ls.TextDocumentSignatureHelp,
		"textDocument/completion":           ls.TextDocumentCompletion,
		"textDocument/formatting":           ls.TextDocumentFormatting,
		"textDocument/inlayHint":            ls.TextDocumentInlayHint,
		"completionItem/resolve":            ls.CompletionItemResolve,
		"textDocument/diagnostic":           ls.TextDocumentDiagnostic,
		"textDocument/definition":           ls.TextDocumentDefinition,
		"textDocument/references":           ls.TextDocumentReferences,
	}
}

func (ls *BmathLanguageServer) Initialize(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	server.Initialize()
	return &lsp.InitializeResult{
		Capabilities: lsp.ServerCapabilities{
			DefinitionProvider:         util.Ptr(true),  // TODO: C: int foo = 10; -- support this
			DeclarationProvider:        util.Ptr(false), // C: int foo;
			TypeDefinitionProvider:     util.Ptr(false), // C: struct something {};
			ImplementationProvider:     util.Ptr(false), // C: int something() { code(); }
			ReferencesProvider:         util.Ptr(true),
			DocumentFormattingProvider: util.Ptr(true),
			InlayHintProvider:          util.Ptr(true),
			DiagnosticProvider: &lsp.DiagnosticOptions{
				WorkspaceDiagnostics:  false,
				InterFileDependencies: false,
			},
			TextDocumentSync: &lsp.TextDocumentSyncOptions{
				OpenClose: util.Ptr(true),
				Change:    util.Ptr(lsp.Full),
			},
			SemanticTokensProvider: &lsp.SemanticTokensProvider{
				SemanticTokensOptions: &lsp.SemanticTokensOptions{
					WorkDoneProgressOptions: lsp.WorkDoneProgressOptions{
						WorkDoneProgress: util.Ptr(false), // TODO: Server needs to do something about workDoneProgress for interesting functionality such as formatting
					},
					Legend: lsp.SemanticTokensLegend{
						TokenTypes:     SemanticTokens,
						TokenModifiers: []string{},
					},
					Range: util.Ptr(true),
					Full: &lsp.Delta{
						Delta: util.Ptr(false), // TODO: TextDocumentSemanticTokensFullDelta
					},
				},
				SemanticTokensRegistration: nil,
			},
			DocumentSymbolProvider: util.Ptr(true),
			SignatureHelpProvider: &lsp.SignatureHelpOptions{
				TriggerCharacters: &[]string{"K"},
			},
			CompletionProvider: &lsp.CompletionOptions{
				WorkDoneProgressOptions: lsp.WorkDoneProgressOptions{
					WorkDoneProgress: util.Ptr(false),
				},
				// TODO: Need to check client capabilities to send AllCommitCharacters instead
				// TriggerCharacters: &[]string{"@"},
				ResolveProvider: util.Ptr(true),
				CompletionItem: &lsp.CompletionItem{
					LabelDetailsSupport: util.Ptr(true),
				},
			},
		},
		ServerInfo: &lsp.ServerInfo{
			Name:    "bmath-lsp",
			Version: util.Ptr("0.0.1"),
		},
	}, nil
}

func (ls *BmathLanguageServer) FetchDocument(requestData *v2.Request) (*Document, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	textDocument, ok := params["textDocument"].(map[string]any)
	if !ok {
		return nil, errors.New("$.textDocument couldn't be parsed")
	}

	uri, ok := textDocument["uri"].(string)
	if !ok {
		return nil, errors.New("$.textDocument.uri couldn't be parsed")
	}

	document, ok := ls.documents[uri]
	if !ok {
		return nil, fmt.Errorf("document %q closed", uri)
	}

	return document, nil
}

func (ls *BmathLanguageServer) FetchRange(params map[string]any, prefix string) (*lsp.Range, error) {
	rangeParam, ok := params["range"].(map[string]any)
	if !ok {
		return nil, fmt.Errorf("%s.range couldn't be parsed", prefix)
	}

	start, ok := rangeParam["start"].(map[string]any)
	if !ok {
		return nil, fmt.Errorf("%s.range.start couldn't be parsed", prefix)
	}

	end, ok := rangeParam["end"].(map[string]any)
	if !ok {
		return nil, fmt.Errorf("%s.range.end couldn't be parsed", prefix)
	}

	startLine, ok := start["line"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.range.start.line couldn't be parsed", prefix)
	}

	startCharacter, ok := start["character"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.range.start.character couldn't be parsed", prefix)
	}

	endLine, ok := end["line"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.range.end.line couldn't be parsed", prefix)
	}

	endCharacter, ok := end["character"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.range.end.character couldn't be parsed", prefix)
	}

	return &lsp.Range{
		Start: lsp.Position{
			Line:      uint(startLine),
			Character: uint(startCharacter),
		},
		End: lsp.Position{
			Line:      uint(endLine),
			Character: uint(endCharacter),
		},
	}, nil
}

func (ls *BmathLanguageServer) FetchPosition(params map[string]any, prefix string) (*lsp.Position, error) {
	position, ok := params["position"].(map[string]any)
	if !ok {
		return nil, fmt.Errorf("%s.position couldn't be parsed", prefix)
	}

	line, ok := position["line"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.position.line couldn't be parsed", prefix)
	}

	character, ok := position["character"].(float64)
	if !ok {
		return nil, fmt.Errorf("%s.position.character couldn't be parsed", prefix)
	}

	return &lsp.Position{
		Line:      uint(line),
		Character: uint(character),
	}, nil
}

func (ls *BmathLanguageServer) Initialized(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	// Optionally send back a client/registerCapability to the client to negotiate capabilities
	// see: https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#initialized
	// see: https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#client_registerCapability
	return nil, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didOpen
func (ls *BmathLanguageServer) TextDocumentDidOpen(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	textDocument, ok := params["textDocument"].(map[string]any)
	if !ok {
		return nil, errors.New("$.textDocument couldn't be parsed")
	}

	uri, ok := textDocument["uri"].(string)
	if !ok {
		return nil, errors.New("$.textDocument.uri couldn't be parsed")
	}

	version, ok := textDocument["version"].(float64)
	if !ok {
		return nil, errors.New("$.textDocument.version couldn't be parsed")
	}

	languageID, ok := textDocument["languageId"].(string)
	if !ok {
		return nil, errors.New("$.textDocument.languageId couldn't be parsed")
	}

	text, ok := textDocument["text"].(string)
	if !ok {
		return nil, errors.New("$.textDocument.text couldn't be parsed")
	}

	_, ok = ls.documents[uri]
	if ok {
		return nil, fmt.Errorf("document %q already opened", uri)
	}

	document := NewDocument(uri, LanguageID(languageID))
	document.Commit(int(version), text)
	ls.documents[uri] = document
	return nil, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didClose
// notification
func (ls *BmathLanguageServer) TextDocumentDidClose(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	delete(ls.documents, document.URI())
	return nil, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_didChange
// notification
func (ls *BmathLanguageServer) TextDocumentDidChange(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	textDocument, ok := params["textDocument"].(map[string]any)
	if !ok {
		return nil, errors.New("$.textDocument couldn't be parsed")
	}

	uri, ok := textDocument["uri"].(string)
	if !ok {
		return nil, errors.New("$.textDocument.uri couldn't be parsed")
	}

	version, ok := textDocument["version"].(float64)
	if !ok {
		return nil, errors.New("$.textDocument.version couldn't be parsed")
	}

	contentChanges, ok := params["contentChanges"].([]any)
	if !ok {
		return nil, errors.New("$.contentChanges couldn't be parsed")
	}

	// TODO: I this language server doens't deal in range-changes right now, just full changes
	// therefore, the content changes are just the {text: string}

	document, ok := ls.documents[uri]
	if !ok {
		return nil, fmt.Errorf("document %q not opened", uri)
	}

	numChanges := len(contentChanges)
	if numChanges == 0 {
		return nil, nil
	}

	lastChange, ok := contentChanges[numChanges-1].(map[string]any)
	if !ok {
		return nil, fmt.Errorf("contentChanges[%d] couldn't be parsed", numChanges-1)
	}

	changeText, ok := lastChange["text"].(string)
	if !ok {
		return nil, fmt.Errorf("contentChanges[%d].text couldn't be parsed", numChanges-1)
	}

	document.Commit(int(version), changeText)

	return nil, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_semanticTokens
func (ls *BmathLanguageServer) TextDocumentSemanticTokensFull(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	semanticTokens, err := document.SemanticTokensFull(ls.lexer)
	if err != nil {
		return nil, err
	}

	return &lsp.SemanticTokens{
		Data: semanticTokens,
	}, nil
}

func (ls *BmathLanguageServer) TextDocumentSemanticTokensRange(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	r, err := ls.FetchRange(params, "$")
	if err != nil {
		return nil, err
	}

	semanticTokens, err := document.SemanticTokensRange(ls.lexer, r)
	if err != nil {
		return nil, err
	}

	return &lsp.SemanticTokens{
		Data: semanticTokens,
	}, nil
}

// TextDocumentDocumentSymbol returns an array of variables being assigned
// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_documentSymbol
func (ls *BmathLanguageServer) TextDocumentDocumentSymbol(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	symbols, err := document.Symbols(ls.lexer)
	if err != nil {
		return nil, err
	}

	return symbols, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_signatureHelp
func (ls *BmathLanguageServer) TextDocumentSignatureHelp(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	position, err := ls.FetchPosition(params, "$")
	if err != nil {
		return nil, err
	}

	// TODO: There's another case where client can have textDocument.signatureHelp.contextSupport = true capability set that may need to be hanlded
	// this comes with additional request parameters such as context?: SignatureHelpContext

	// TODO: This gets interesting if I add user-defined functions, until then...
	signatures := make([]lsp.SignatureInformation, 0, 32)
	// TODO: Use the ls.lexer.EnumerateTokens() instead. We'll be able to know if character is position is on an argument to highlight that specific argument in the client's thing
	iterator, iteratorErr := ls.lexer.EnumerateSymbols(document.NewReaderFromPositionFuzzy(position))
	for symbolResult := range iterator {
		if symbolResult.Dubious() {
			continue
		}

		sym := symbolResult.Item
		if sym.Type != SymbolTypeFunction || position.Character < uint(sym.Token.Offset) || position.Character > uint(sym.Token.Offset+sym.Token.Length) {
			continue
		}

		signature, ok := Documentation[sym.Identifier]
		if !ok {
			continue
		}

		// active parameter is interesting... it's not calculable atm
		signature.ActiveParameter = util.Ptr(uint(0))
		signatures = append(signatures, *signature)
	}

	if iteratorErr() != nil {
		return nil, iteratorErr()
	}

	if len(signatures) < 1 {
		return nil, nil
	}

	return &lsp.SignatureHelp{
		Signatures: signatures,
		// bmath functions do not have more than one signature for a function, stay at 0
		ActiveSignature: util.Ptr(uint(0)),
	}, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_formatting
func (ls *BmathLanguageServer) TextDocumentFormatting(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	options, ok := params["options"].(map[string]any)
	if !ok {
		return nil, errors.New("$.options couldn't be parsed")
	}

	// Tab size and insert spaces is irrelevant for bmath, but not spec
	_, ok = options["tabSize"].(float64)
	if !ok {
		return nil, errors.New("$.options.tabSize couldn't be parsed")
	}

	_, ok = options["insertSpaces"].(bool)
	if !ok {
		return nil, errors.New("$.options.insertSpaces couldn't be parsed")
	}

	// IDK how nil works
	trimTrailingWhitespace := true
	value, hasTrimTrailingWhitespace := options["trimTrailingWhitespace"]
	if hasTrimTrailingWhitespace {
		trimTrailingWhitespace, ok = value.(bool)
		if !ok {
			return nil, errors.New("$.options.trimTrailingWhitespace couldn't be parsed")
		}
	}

	insertFinalNewline := true
	value, hasInsertFinalNewline := options["insertFinalNewline"]
	if hasInsertFinalNewline {
		insertFinalNewline, ok = value.(bool)
		if !ok {
			return nil, errors.New("$.options.insertFinalNewline couldn't be parsed")
		}
	}

	trimFinalNewlines := true
	value, hasTrimFinalNewlines := options["trimFinalNewlines"]
	if hasTrimFinalNewlines {
		trimFinalNewlines, ok = value.(bool)
		if !ok {
			return nil, errors.New("$.options.trimFinalNewlines couldn't be parsed")
		}
	}

	// other things???? IDK what the client would send over nor would the server know what to send it to support...
	formatSettings := &FormatSettings{
		TrimTrailingWhitespace: trimTrailingWhitespace,
		AddNewLineAtEndOfFile:  insertFinalNewline,
		TrimFinalNewlines:      trimFinalNewlines,
	}

	edits, err := document.Format(NewFormatter(ls.lexer, formatSettings))
	if err != nil {
		return nil, err
	}

	if len(edits) <= 0 {
		return nil, nil
	}

	return edits, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_completion
func (ls *BmathLanguageServer) TextDocumentCompletion(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	return nil, errors.New("TextDocumentCompletion not implemented; TODO")
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionItem_resolve
func (ls *BmathLanguageServer) CompletionItemResolve(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	return nil, errors.New("CompletionItemResolve not implemented; TODO")
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_definition
func (ls *BmathLanguageServer) TextDocumentDefinition(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	position, err := ls.FetchPosition(params, "$")
	if err != nil {
		return nil, err
	}

	targetSymbol, err := document.SymbolAtPosition(ls.lexer, position)
	if err != nil {
		return nil, err
	}

	if targetSymbol == nil {
		return nil, nil
	}

	var prevSymbol *Symbol
	var locations []*lsp.Location

	iterator, iteratorErr := ls.lexer.EnumerateTokens(document.NewReader())
	for tokenResult := range iterator {
		token := tokenResult.Item
		if tokenResult.Dubious() {
			continue
		}

		symbol, ok := token.ToSymbol()
		if ok {
			prevSymbol = symbol
		}

		if !token.Is(TokenTypeAssignment) {
			continue
		}

		if prevSymbol == nil {
			continue
		}

		if prevSymbol.Identifier != targetSymbol.Identifier {
			prevSymbol = nil
			continue
		}

		locations = append(locations, &lsp.Location{
			Uri:   document.URI(),
			Range: *prevSymbol.Token.Range(),
		})
	}

	if iteratorErr() != nil {
		return nil, iteratorErr()
	}

	return locations, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_references
func (ls *BmathLanguageServer) TextDocumentReferences(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	position, err := ls.FetchPosition(params, "$")
	if err != nil {
		return nil, err
	}

	targetSymbol, err := document.SymbolAtPosition(ls.lexer, position)
	if err != nil {
		return nil, err
	}

	if targetSymbol == nil {
		return nil, nil
	}

	var locations []*lsp.Location

	iterator, iteratorErr := ls.lexer.EnumerateSymbols(document.NewReader())
	for symbolResult := range iterator {
		symbol := symbolResult.Item
		if symbolResult.Dubious() {
			continue
		}

		if symbol.Identifier != targetSymbol.Identifier {
			continue
		}

		locations = append(locations, &lsp.Location{
			Uri:   document.URI(),
			Range: *symbol.Token.Range(),
		})
	}

	if iteratorErr() != nil {
		return nil, iteratorErr()
	}

	return locations, nil
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_inlayHint
func (ls *BmathLanguageServer) TextDocumentInlayHint(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	params, ok := requestData.Params.(map[string]any)
	if !ok {
		return nil, errors.New("request params couldn't be parsed")
	}

	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	textRange, err := ls.FetchRange(params, "$")
	if err != nil {
		return nil, err
	}

	return document.InlayHints(ls.lexer, textRange)
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocument_diagnostic
func (ls *BmathLanguageServer) TextDocumentDiagnostic(ctx context.Context, server *lsp.Server, requestData *v2.Request) (any, error) {
	document, err := ls.FetchDocument(requestData)
	if err != nil {
		return nil, err
	}

	diagnosticItems := make([]lsp.Diagnostic, 0, 32)

	iter, iterErr := ls.lexer.EnumerateTokens(document.NewReader())
	for tokenResult := range iter {
		if !tokenResult.HasError() || tokenResult.Dubious() {
			continue
		}

		diagItem := lsp.Diagnostic{
			Range:    *tokenResult.Item.Range(),
			Severity: util.Ptr(lsp.DiagnosticSeverityError),
			Source:   util.Ptr("lexer"),
			Message:  tokenResult.Error.Error(),
		}

		if tokenResult.Item.Is(TokenTypeIdentifier) {
			sym, ok := tokenResult.Item.ToSymbol()
			if ok {
				diagItem.Message = fmt.Errorf("%w: %s", tokenResult.Error, sym.Identifier).Error()
			}
		}

		diagnosticItems = append(diagnosticItems, diagItem)
	}

	if iterErr() != nil {
		return nil, iterErr()
	}

	return &lsp.RelatedFullDocumentDiagnosticsReport{
		FullDocumentDiagnosticsReport: &lsp.FullDocumentDiagnosticsReport{
			Kind:  lsp.DiagnosticReportKindFull,
			Items: diagnosticItems,
		},
	}, nil
}
