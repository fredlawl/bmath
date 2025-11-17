import sys

import pytest
from lsprotocol.types import (
    ClientCapabilities,
    CompletionList,
    CompletionParams,
    DidOpenTextDocumentParams,
    InitializeParams,
    Position,
    TextDocumentIdentifier,
)

import pytest_lsp
from pytest_lsp import ClientServerConfig, LanguageClient


@pytest_lsp.fixture(
    scope="module",
    config=ClientServerConfig(
        server_command=[
            "/usr/local/go/bin/go",
            "run",
            "/home/fred/Projects/bmath/bmath-server/cmd/main.go",
            "--debug",
        ]
    ),
)
async def client(lsp_client: LanguageClient):
    # Setup
    params = InitializeParams(capabilities=ClientCapabilities())
    await lsp_client.initialize_session(params)

    yield

    # Teardown
    await lsp_client.shutdown_session()


@pytest.mark.asyncio
async def test_sanity(client: LanguageClient):
    """Ensure that the server implements completions correctly."""
    assert True


@pytest.mark.asyncio
async def test_completions(client: LanguageClient):
    """Ensure that the server implements completions correctly."""

    results = await client.text_document_completion_async(
        params=CompletionParams(
            position=Position(line=1, character=0),
            text_document=TextDocumentIdentifier(uri="file:///path/to/file.txt"),
        )
    )
    assert results is not None

    if isinstance(results, CompletionList):
        items = results.items
    else:
        items = results

    labels = [item.label for item in items]
    assert labels == ["hello", "world"]
