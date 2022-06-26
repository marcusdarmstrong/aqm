FROM node:18
RUN apt update -y && apt-get install -y avahi-utils
COPY . .
CMD ["node", "mdns-publisher.js"]