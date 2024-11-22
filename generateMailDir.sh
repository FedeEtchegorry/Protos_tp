#!/bin/bash

MAILDIR="mailDir"
mkdir -p "$MAILDIR"

NUM_USERS=10

for i in $(seq 0 $((NUM_USERS-1))); do
    USER_DIR="$MAILDIR/user$i"
    mkdir -p "$USER_DIR/new" "$USER_DIR/cur" "$USER_DIR/tmp"

    for j in $(seq 1 3); do
        echo "Subject: Test Email $j" > "$USER_DIR/new/mail$j.txt"
        echo "From: sender$j@example.com" >> "$USER_DIR/new/mail$j.txt"
        echo "To: user$i@example.com" >> "$USER_DIR/new/mail$j.txt"
        echo "" >> "$USER_DIR/new/mail$j.txt"
        echo "This is the body of test email $j for user$i." >> "$USER_DIR/new/mail$j.txt"
    done
done

echo "MailDir structure created at $MAILDIR with $NUM_USERS users and example mails."