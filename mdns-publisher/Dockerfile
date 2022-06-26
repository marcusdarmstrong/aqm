FROM node:18-buster
RUN apt update -y && apt-get install -y avahi-utils
COPY . .
CMD ["node", "mdns-publisher.js"]