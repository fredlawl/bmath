package lsp

type TextDocumentSyncKind int

const (
	None TextDocumentSyncKind = iota
	Full
	Incremental
)

type WorkDoneProgressOptions struct {
	WorkDoneProgress *bool `json:"workDoneProgress,omitempty"`
}

type SemanticTokensLegend struct {
	TokenTypes     []string `json:"tokenTypes"`
	TokenModifiers []string `json:"tokenModifiers"`
}

type Delta struct {
	*bool
	Delta *bool `json:"delta,omitempty"`
}

type SemanticTokensOptions struct {
	WorkDoneProgressOptions
	Legend SemanticTokensLegend `json:"legend"`
	Range  *bool                `json:"range,omitempty"`
	Full   *Delta               `json:"full,omitempty"`
}

type DocumentFilter struct {
	Language *string `json:"language,omitempty"`
	Scheme   *string `json:"scheme,omitempty"`
	Pattern  *string `json:"pattern,omitempty"`
}

type (
	DocumentSelector                []DocumentFilter
	TextDocumentRegistrationOptions struct {
		DocumentSelector *DocumentSelector `json:"documentSelector"`
	}
)

type StaticRegistrationOptions struct {
	Id *string `json:"id,omitempty"`
}

type SemanticTokensRegistration struct {
	TextDocumentRegistrationOptions
	SemanticTokensOptions
	StaticRegistrationOptions
}

type TextDocumentSyncOptions struct {
	OpenClose *bool                 `json:"openClose,omitempty"`
	Change    *TextDocumentSyncKind `json:"change,omitempty"`
}

type SemanticTokensProvider struct {
	*SemanticTokensOptions
	*SemanticTokensRegistration
}

type CompletionItem struct {
	LabelDetailsSupport *bool `json:"labelDetailsSupport,omitempty"`
}

type CompletionOptions struct {
	WorkDoneProgressOptions
	TriggerCharacters   *[]string       `json:"triggerCharacters,omitempty"`
	AllCommitCharacters *[]string       `json:"allCommitCharacters,omitempty"`
	ResolveProvider     *bool           `json:"resolveProvider,omitempty"`
	CompletionItem      *CompletionItem `json:"completionItem,omitempty"`
}

type DiagnosticOptions struct {
	*WorkDoneProgressOptions
	Identifier            *string `json:"identifier,omitempty"`
	InterFileDependencies bool    `json:"interFileDependencies,omitempty"`
	WorkspaceDiagnostics  bool    `json:"workspaceDiagnostics,omitempty"`
}

type ServerCapabilities struct {
	TextDocumentSync       *TextDocumentSyncOptions `json:"textDocumentSync,omitempty"`
	SemanticTokensProvider *SemanticTokensProvider  `json:"semanticTokensProvider,omitempty"`
	// DocumentSymbolProvider *DocumentSymbolProvider  `json:"documentSymbolProvider,omitempty"`
	DocumentSymbolProvider     *bool                 `json:"documentSymbolProvider,omitempty"`
	SignatureHelpProvider      *SignatureHelpOptions `json:"signatureHelpProvider,omitempty"`
	ImplementationProvider     *bool                 `json:"implementationProvider,omitempty"`
	CompletionProvider         *CompletionOptions    `json:"completionProvider,omitempty"`
	DocumentFormattingProvider *bool                 `json:"documentFormattingProvider,omitempty"`
	TypeDefinitionProvider     *bool                 `json:"typeDefinitionProvider,omitempty"`
	DeclarationProvider        *bool                 `json:"declarationProvider,omitempty"`
	DefinitionProvider         *bool                 `json:"definitionProvider,omitempty"`
	ReferencesProvider         *bool                 `json:"referencesProvider,omitempty"`
	InlayHintProvider          *bool                 `json:"inlayHintProvider,omitempty"`
	DiagnosticProvider         *DiagnosticOptions    `json:"diagnosticProvider,omitempty"`
}

type ServerInfo struct {
	Name    string  `json:"name"`
	Version *string `json:"version,omitempty"`
}

type InitializeResult struct {
	Capabilities ServerCapabilities `json:"capabilities"`
	ServerInfo   *ServerInfo        `json:"serverInfo,omitempty"`
}

type DocumentUri string

type TextDocumentItem struct {
	Uri        DocumentUri `json:"uri"`
	LanguageId string      `json:"languageId"`
	Version    int         `json:"version"`
	Text       string      `json:"text"`
}

type ProgressToken struct {
	*int
	*string
}

type PartialResultParams struct {
	PartiallResultToken *ProgressToken `json:"partialResultToken,omitempty"`
}

type TextDocumentIdentifier struct {
	Uri DocumentUri `json:"uri"`
}

type SemanticTokenParams struct {
	WorkDoneProgressOptions
	PartialResultToken *ProgressToken         `json:"partialResultToken"`
	TextDocument       TextDocumentIdentifier `json:"textDocument"`
}

type SemanticTokens struct {
	ResultId *string `json:"resultId,omitempty"`
	Data     []uint  `json:"data"`
}

type SemanticTokensPartialResult struct {
	Data []uint `json:"data"`
}

type DocumentSymbolOptions struct {
	WorkDoneProgressOptions
	Label *string `json:"label,omitempty"`
}

type DocumentSymbolProvider struct {
	*bool
	*DocumentSymbolOptions
}

type SymbolKind int

const (
	SymbolKindFile SymbolKind = iota + 1
	SymbolKindModule
	SymbolKindNamespace
	SymbolKindPackage
	SymbolKindClass
	SymbolKindMethod
	SymbolKindProperty
	SymbolKindField
	SymbolKindConstructor
	SymbolKindEnum
	SymbolKindInterface
	SymbolKindFunction
	SymbolKindVariable
	SymbolKindConstant
	SymbolKindString
	SymbolKindNumber
	SymbolKindBoolean
	SymbolKindArray
	SymbolKindObject
	SymbolKindKey
	SymbolKindNull
	SymbolKindEnumMember
	SymbolKindStruct
	SymbolKindEvent
	SymbolKindOperator
	SymbolKindTypeParameter
)

type SymbolTag int

const (
	SymbolTagDeprecated SymbolTag = iota + 1
)

type Position struct {
	Line      uint `json:"line"`
	Character uint `json:"character"`
}

type Range struct {
	Start Position `json:"start"`
	End   Position `json:"end"`
}

type DocumentSymbol struct {
	Name   string      `json:"name"`
	Detail *string     `json:"detail,omitempty"`
	Kind   SymbolKind  `json:"kind"`
	Tags   []SymbolTag `json:"tags,omitempty"`
	// Deprecated: Field Deprecated is deprecated use Tags instead
	Deprecated     *bool             `json:"deprecated,omitempty"`
	Range          Range             `json:"range"`
	SelectionRange Range             `json:"selectionRange"`
	Children       []*DocumentSymbol `json:"children,omitempty"`
}

type SignatureHelpOptions struct {
	WorkDoneProgressOptions
	TriggerCharacters   *[]string `json:"triggerCharacters,omitempty"`
	RetriggerCharacters *[]string `json:"retriggerCharacters,omitempty"`
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#parameterInformation
type ParameterInformation struct {
	Label         string `json:"label"`
	Documentation string `json:"documentation,omitempty"`
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#signatureInformation
type SignatureInformation struct {
	Label           string                  `json:"label"`
	Documentation   *string                 `json:"documentation,omitempty"`
	Parameters      *[]ParameterInformation `json:"parameters,omitempty"`
	ActiveParameter *uint                   `json:"activeParameter,omitempty"`
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#signatureHelp
type SignatureHelp struct {
	Signatures      []SignatureInformation `json:"signatures"`
	ActiveSignature *uint                  `json:"activeSignature,omitempty"`
	// Deprecated: Field ActiveParameter is deprecated use SignatureInformation.ActiveParameter
	ActiveParameter *uint `json:"activeParameter,omitempty"`
}

type TextEdit struct {
	Range   Range  `json:"range"`
	NewText string `json:"newText"`
}

type InlayHintKind uint

const (
	InlayHintKindType InlayHintKind = iota + 1
	InlayHintKindParameter
)

type InlayHint struct {
	Position     Position       `json:"position"`
	Label        string         `json:"label"`
	Kind         *InlayHintKind `json:"kind,omitempty"`
	PaddingRight *bool          `json:"paddingRight,omitempty"`
	PaddingLeft  *bool          `json:"paddingLeft,omitempty"`
	Tooltip      *string        `json:"tooltip,omitempty"`
}

type DocumentDiagnosticReportKind string

const (
	DiagnosticReportKindFull      DocumentDiagnosticReportKind = DocumentDiagnosticReportKind("full")
	DiagnosticReportKindUnchanged DocumentDiagnosticReportKind = DocumentDiagnosticReportKind("unchanged")
)

type DiagnosticSeverity int

const (
	DiagnosticSeverityError DiagnosticSeverity = iota + 1
	DiagnosticSeverityWarning
	DiagnosticSeverityInformation
	DiagnosticSeverityHint
)

type CodeDescription struct {
	Href string `json:"href"` // TODO: Should be url.Url or something
}

type DiagnosticTag int

const (
	DiagnosticTagUncessary DiagnosticTag = iota + 1
	DiagnosticTagDeprecated
)

type Location struct {
	Uri   string `json:"uri"`
	Range Range  `json:"range"`
}

type DiagnosticRelatedInformation struct {
	Location Location `json:"location"`
	Message  string   `json:"message"`
}

// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#diagnostic
type Diagnostic struct {
	Range              Range                           `json:"range"`
	Severity           *DiagnosticSeverity             `json:"severity,omitempty"`
	Code               *string                         `json:"code,omitempty"`
	CodeDescription    *CodeDescription                `json:"codeDescription,omitempty"`
	Source             *string                         `json:"source,omitempty"`
	Message            string                          `json:"message"`
	Tags               *[]DiagnosticTag                `json:"tags,omitempty"`
	RelatedInformation *[]DiagnosticRelatedInformation `json:"relatedInformation,omitempty"`
}

type FullDocumentDiagnosticsReport struct {
	Kind     DocumentDiagnosticReportKind `json:"kind,omitempty"`
	ResultID *string                      `json:"resultId,omitempty"`
	Items    []Diagnostic                 `json:"items"`
}

type RelatedFullDocumentDiagnosticsReport struct {
	*FullDocumentDiagnosticsReport
	RelatedDocuments *map[string]any `json:"relatedDocuments,omitempty"`
}
