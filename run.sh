#!/bin/sh
# Run DRIVE.
#   ./run.sh              start drive_server in its own terminal, then the client
#   ./run.sh server       just the server, in this terminal
#   ./run.sh client HOST  just the client (default host: localhost)
set -e

cd "$(dirname "$0")/DRIVE"
DIR=$(pwd)

if [ ! -x ./drive ] || [ ! -x ./drive_server ]; then
    case "$(uname)" in
    Darwin) ../build.macos.sh ;;
    *)      ../build.linux.sh ;;
    esac
fi

start_server_window() {
    if [ "$(uname)" = Darwin ]; then
        osascript -e "tell application \"Terminal\" to do script \"cd '$DIR' && ./drive_server\"" \
                  -e 'tell application "Terminal" to activate' >/dev/null
    elif command -v x-terminal-emulator >/dev/null; then
        x-terminal-emulator -e "$DIR/drive_server" &
    elif command -v xterm >/dev/null; then
        xterm -e "$DIR/drive_server" &
    else
        echo "No terminal emulator found.  Run './run.sh server' in one window"
        echo "and './run.sh client' in another."; exit 1
    fi
}

case "$1" in
server) exec ./drive_server ;;
client) exec ./drive "${2:-localhost}" ;;
"")
    start_server_window
    sleep 3
    echo "Server console is in the other window."
    echo "Press Return there on NEXT STATE to start the race."
    exec ./drive localhost
    ;;
*) echo "usage: $0 [server | client [host]]"; exit 1 ;;
esac
