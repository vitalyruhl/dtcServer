# GitHub Secrets Configuration

This file documents the secrets needed for GitHub Actions workflows and local development.
**DO NOT commit actual secret values to this file!**

## Coinbase API Credentials (Local Development)

### JWT Authentication (CDP Advanced Trade API)
For local development and testing, create these files in the `secrets/` directory:

```
secrets/
├── cdp_api_key.json     # CDP API credentials (JSON format)
└── coinbase.h           # Legacy format (if needed)
```

**Example `secrets/cdp_api_key.json`:**
```json
{
    "id": "your-cdp-api-key-id",
    "privateKey": "-----BEGIN EC PRIVATE KEY-----\nYour ECDSA P-256 private key\n-----END EC PRIVATE KEY-----"
}
```

**Security Requirements:**
- Use ECDSA P-256 private keys (not Ed25519)
- Store private key in PEM format
- The `secrets/` directory is gitignored for security
- Use environment variables for production: `CDP_API_KEY_ID` and `CDP_PRIVATE_KEY`

## Required Secrets for CI/CD

### Docker Hub (Optional - for publishing Docker images)
- `DOCKER_USERNAME`: Your Docker Hub username
- `DOCKER_PASSWORD`: Your Docker Hub password or access token

### Server Deployment (Optional - for automatic deployment)
- `SERVER_HOST`: IP address or domain of your deployment server
- `SERVER_USER`: SSH username for server access
- `SERVER_SSH_KEY`: Private SSH key for server authentication

## How to Set Up Secrets

### 1. Docker Hub Setup (if you want to publish images)

1. Create account at https://hub.docker.com/
2. Go to Account Settings > Security > New Access Token
3. Create token with "Read, Write, Delete" permissions
4. Copy the token (you won't see it again!)

### 2. Server Setup (if you want auto-deployment)

1. Generate SSH key pair for GitHub Actions:
   ```bash
   ssh-keygen -t rsa -b 4096 -f github_actions_key
   ```
2. Copy public key to your server:
   ```bash
   ssh-copy-id -i github_actions_key.pub user@your-server.com
   ```
3. Keep the private key content for GitHub secrets

### 3. Adding Secrets to GitHub

1. Go to your repository on GitHub
2. Click "Settings" tab
3. In left sidebar, click "Secrets and variables" > "Actions"
4. Click "New repository secret"
5. Add each secret with the exact names listed above

### 4. Testing Secrets

Secrets are automatically available in workflows as `${{ secrets.SECRET_NAME }}`
The workflows will automatically detect if secrets are configured and skip steps if not.

## Example Values (DO NOT USE THESE)

```
DOCKER_USERNAME=myusername
DOCKER_PASSWORD=dckr_pat_1234567890abcdef
SERVER_HOST=192.168.1.100
SERVER_USER=deploy
SERVER_SSH_KEY=-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA...
-----END RSA PRIVATE KEY-----
```

## Security Notes

- Never commit real secrets to version control
- Use access tokens instead of passwords when possible
- Regularly rotate secrets
- Use least-privilege principles for SSH keys
- Consider using GitHub's OIDC for cloud deployments instead of long-lived tokens