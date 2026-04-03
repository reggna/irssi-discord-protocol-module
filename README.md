# irssi-discord

A protocol module for [Irssi](https://irssi.org/) that provides Discord support using the Discord Bot API (v10).

## Features (2026 Update)

- **Discord Protocol Integration:** Native "Discord" chat protocol in Irssi.
- **Discord API v10:** Updated to use the latest Discord REST and Gateway APIs.
- **Real-time Updates:** Uses a background thread and WebSockets (`libcurl`) to receive messages in real-time.
- **Bot Token Authentication:** Automatic `Bot ` prefix handling—just provide your raw token.
- **Message History:** Fetches recent message history upon joining a channel or starting a query, displayed in the correct chronological order.
- **Non-blocking UI:** Real-time messages are processed via a background thread to prevent Irssi's UI from hanging.

## Prerequisites

- **Irssi 1.4+** (headers required for building)
- **libcurl 8.12+** (with WebSocket support enabled)
- **jansson** (JSON library)
- **glib-2.0**
- **pkg-config**, **gcc**, **make**

## Building

1.  Place the Irssi source code in `include/irssi` (or update the `Makefile` to point to your Irssi headers).
2.  Run `make`:
    ```bash
    make
    ```

The compiled module will be at `lib/libdiscord.so`.

## Installation

1.  Copy `lib/libdiscord.so` to your Irssi modules directory (usually `~/.irssi/modules/`).
2.  In Irssi, load the module:
    ```
    /load discord
    ```

## Configuration

1.  **Add the Network:**
    ```
    /NETWORK ADD Discord
    ```

2.  **Add the Server:**
    ```
    /SERVER ADD -network Discord gateway.discord.gg 443 <YOUR_BOT_TOKEN>
    ```

3.  **Connect:**
    ```
    /CONNECT Discord
    ```

## Usage

- **Join a Channel:** Use the Channel ID.
  ```
  /JOIN <CHANNEL_ID>
  ```
- **Direct Message:** Use the User ID or DM Channel ID.
  ```
  /QUERY <ID>
  ```

## Technical Details

- **Gateway:** The module connects to the Discord Gateway via WebSockets. It currently supports `MESSAGE_CREATE` events. Heartbeating and advanced reconnection logic are still experimental.
- **Threading:** A background GThread handles the WebSocket connection, pushing messages to a thread-safe queue which is then drained by Irssi's main loop via `g_idle_add`.

