#!/bin/bash
set -e

echo "Installing dependencies..."
sudo apt update
sudo apt install -y curl wget unzip

echo "Downloading latest Loki release for arm64..."
LOKI_VERSION=$(curl -s "https://api.github.com/repos/grafana/loki/releases/latest" | grep -Po '"tag_name": "\K[^"]*')
echo "Latest version: $LOKI_VERSION"

wget -q "https://github.com/grafana/loki/releases/download/${LOKI_VERSION}/loki-linux-arm64.zip"
unzip -o loki-linux-arm64.zip
chmod +x loki-linux-arm64
sudo mv loki-linux-arm64 /usr/local/bin/loki
rm loki-linux-arm64.zip

echo "Creating directories..."
sudo mkdir -p /etc/loki
sudo mkdir -p /var/lib/loki/chunks
sudo mkdir -p /var/lib/loki/rules

echo "Fixing permissions..."
sudo chown -R $(whoami):$(whoami) /var/lib/loki
sudo chown -R $(whoami):$(whoami) /etc/loki

echo "Copying config and service files..."
sudo cp config.yaml /etc/loki/config.yaml
sudo cp loki.service /etc/systemd/system/loki.service

echo "Starting Loki service..."
sudo systemctl daemon-reload
sudo systemctl enable loki
sudo systemctl start loki

echo "Waiting for Loki to become ready..."
sleep 15

echo "Done. Checking status:"
curl -s http://localhost:3100/ready
echo ""
sudo systemctl status loki --no-pager
