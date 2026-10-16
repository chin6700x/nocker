#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ Dockerfile: Full-Stack Web + PHP + Database Recipe for Nocker (Not Docker)
#

# 1. Base Image: Lightweight Alpine Linux ARM64
FROM library/alpine:latest

# 2. Install PHP 8 and SQLite PDO extensions
RUN apk add --no-cache \
    php82 \
    php82-sqlite3 \
    php82-pdo_sqlite \
    php82-session \
    php82-json \
    curl

# 3. Create app and persistent data directories
WORKDIR /app
RUN mkdir -p /app /data

# 4. Copy Web Assets
COPY ./www /app

# 5. Expose Port 8080 to Host
EXPOSE 8080

# 6. Declare Persistent Volume for Database
VOLUME ["/data"]

# 7. Start PHP Web Server
CMD ["php82", "-S", "0.0.0.0:8080", "-t", "/app"]
