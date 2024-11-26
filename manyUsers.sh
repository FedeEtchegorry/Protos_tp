#!/bin/bash

send_request() {
   curl -u user1@example.com.ar:pass pop3://localhost:1080
   curl -u user2@example.com.ar:pass pop3://localhost:1080
   curl -u user3@example.com.ar:pass pop3://localhost:1080
   curl -u user4@example.com.ar:pass pop3://localhost:1080
   curl -u user5@example.com.ar:pass pop3://localhost:1080
   curl -u user6@example.com.ar:pass pop3://localhost:1080
   curl -u user7@example.com.ar:pass pop3://localhost:1080
   curl -u user8@example.com.ar:pass pop3://localhost:1080
   curl -u user9@example.com.ar:pass pop3://localhost:1080
   curl -u user10@example.com.ar:pass pop3://localhost:1080
}
for i in {1..50}; do
  send_request &
done

wait
echo "All requests completed."
