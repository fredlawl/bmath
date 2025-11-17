import * as path from 'path';
import * as vscode from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
  const serverOptions: ServerOptions = {
    command: '/home/fred/Projects/bmath/build/bmath-ls',
    args: ["-debug", "-log", "bmath-ls.log"],
    options: {
      cwd: context.extensionPath
      //cwd: '/home/fred/Projects/bmath/bmath-server/bin'
    }
  }

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'bmath' }]
  };

  client = new LanguageClient(
    'bmath-language-server',
    'Bmath Language Server',
    serverOptions,
    clientOptions
  );

  client.start();
}

export function deactivate() {
  return client?.stop();
}
