# Security policy

## Supported versions

The latest default branch is the only actively supported development line until a release policy is published.

## Reporting a vulnerability

Please do not disclose security vulnerabilities in a public issue. Use a private GitHub Security Advisory or the private vulnerability-reporting mechanism of the host where this repository is published.

Include only the information needed to reproduce and assess the issue. Do not attach personal PDFs, credentials, access tokens, private keys, database files, or other personal data.

If a private reporting channel is not available, contact a repository maintainer privately and request one. A public issue can be used only to ask for a private channel, not to describe the vulnerability.

## Security expectations

Lumina Reader treats PDFs as untrusted input. Contributions must not:

- execute JavaScript embedded in PDFs;
- download external resources without explicit user action;
- transmit documents or reading data by default;
- log passwords, tokens, document contents, or unnecessary personal paths;
- delete original files without explicit confirmation;
- bypass path validation or resource limits.

Security fixes should include regression coverage where practical and document any user-visible migration or mitigation.
