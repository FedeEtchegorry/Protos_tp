#!/bin/bash

send_request() {
   nc localhost -C 1080
}
for i in {1..150}; do
  send_request &
done

wait
echo "All requests completed."
