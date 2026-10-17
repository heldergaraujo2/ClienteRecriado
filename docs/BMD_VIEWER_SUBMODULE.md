# External BMD viewer (upstream submodule)

This repository tracks the upstream project as a Git submodule at
`tools/third_party/muonline-bmd-viewer`, pinned to commit
`3f11bf89f3702261174d8efa1436d56a15c7d97a`.

Upstream: https://github.com/xulek/muonline-bmd-viewer
Upstream describes version 1.1.0 and an ISC license in its README. Review the upstream repository and license before redistributing builds.

## Initialize or update on Windows

Run these commands from the root of `D:\MU-REENGINEER` in PowerShell. Copy only the commands inside the code block.

```powershell
Set-Location "D:\MU-REENGINEER"
git status --short --branch
git submodule sync --recursive
git submodule update --init --recursive
```

If the repository itself has not yet been cloned, clone the project branch first and then initialize its submodules:

```powershell
git clone --branch arena/1d9c1f53-clienterecriado https://github.com/heldergaraujo2/ClienteRecriado.git D:\MU-REENGINEER
Set-Location "D:\MU-REENGINEER"
git submodule update --init --recursive
```

## Run the viewer

Requirements documented upstream: Node.js 22+ and npm. After initializing the submodule:

```powershell
Set-Location "D:\MU-REENGINEER\tools\third_party\muonline-bmd-viewer"
node --version
npm --version
npm ci
npm run dev
```

Open the localhost URL printed by Vite (typically `http://localhost:5173`). To try the desktop shell, inspect the upstream package scripts first; its README documents `npm run electron`.

## First validation plan

1. Open the known plaintext BMD v0x0A test asset `agnecklace.bmd` from the local original-client data directory; do not copy proprietary game assets into this repository.
2. Test with its matching texture folder available locally.
3. Record whether the mesh is visible, whether texture application succeeds, and any console/parser errors.
4. Compare results with the experimental native D3D11 preview in this repository.

## Important scope and safety

- This is a standalone web/Electron tool, not a drop-in C++/Direct3D renderer. Keep it as a diagnostic/reference tool first; port or adapt parser logic only after validating output and license.
- The submodule is pinned to a specific upstream commit for reproducibility. Update it intentionally, not automatically.
- Use plaintext, supported test assets first. Do not use its crypto/decryption features to bypass protected containers. BMD v0x0C payload cipher and Enc* terrain cipher remain blocked by this project's `AGENTS.md`.
- Never commit original client assets, proprietary textures, credentials, or generated private game data.
