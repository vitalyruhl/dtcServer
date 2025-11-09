# SSH Setup for Server Deployment (Optional/Legacy)

> **⚠️ NOTE**: With Docker containerization working, SSH-based deployment is now optional. Most users should use Docker containers directly via `docker-compose` or container orchestration platforms.

This file documents SSH key setup for traditional server deployment via GitHub Actions.
**Use this only if you need to deploy directly to a remote server via SSH.**

## Modern Alternatives (Recommended)

- **Docker Hub**: Push containers to registry, pull on server
- **Container Platforms**: Use Kubernetes, Docker Swarm, or cloud container services  
- **GitHub Container Registry**: `ghcr.io` for container distribution

## SSH Key Setup (Traditional Deployment)

Commands to generate SSH key for GitHub Actions server deployment:
Run these on your local machine (Git Bash, PowerShell, or Command Prompt)

# Generate SSH key pair
ssh-keygen -t rsa -b 4096 -f github_actions_key -C "github-actions@coinbase-dtc-core"

# This creates two files:
# - github_actions_key (private key - for GitHub secret)
# - github_actions_key.pub (public key - for server)

# Copy public key to your server
ssh-copy-id -i github_actions_key.pub user@your-server.com

# Or manually copy the public key content to ~/.ssh/authorized_keys on server

# Display private key content (copy this to GitHub secret)
cat github_actions_key

# Display public key content (copy this to server)
cat github_actions_key.pub