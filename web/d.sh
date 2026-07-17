#!/bin/sh
# TLS trust-store diagnostic for SteamOS. Collects non-sensitive system info
# about the certificate configuration and posts it to a paste service so it
# can be read on another machine. Nothing personal is collected: no
# usernames, no keys, no network identifiers.
OUT=/tmp/inktf-diag.txt
{
    echo "== date =="; date
    echo "== os =="; cat /etc/os-release 2>/dev/null | head -n 4
    echo "== curl version =="; curl -V 2>&1 | head -n 2
    echo "== ssl certs dir =="; ls -l /etc/ssl/certs/ca-certificates.crt 2>&1
    ls -ld /etc/ssl/certs 2>&1
    echo "== extracted dir =="; ls -l /etc/ca-certificates/extracted/ 2>&1
    echo "== p11-kit bundle test =="
    curl --cacert /etc/ca-certificates/extracted/tls-ca-bundle.pem -sSI https://github.com 2>&1 | head -n 2
    echo "== default test =="
    curl -sSI https://github.com 2>&1 | head -n 2
    echo "== explicit symlinked bundle test =="
    curl --cacert /etc/ssl/certs/ca-certificates.crt -sSI https://github.com 2>&1 | head -n 2
    echo "== curl-config ca =="; curl-config --ca 2>&1
    echo "== env =="; env | grep -i -E "^(CURL|SSL)_" 2>/dev/null
} > "$OUT" 2>&1

echo ""
echo "----- diagnostics collected, posting... -----"
if command -v nc > /dev/null 2>&1; then
    URL=$(nc termbin.com 9999 < "$OUT")
    echo "READ THIS BACK: $URL"
else
    URL=$(curl -k -sS -F "file=@$OUT" https://0x0.st)
    echo "READ THIS BACK: $URL"
fi
