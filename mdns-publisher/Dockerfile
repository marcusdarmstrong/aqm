FROM node:18
RUN apt-get install -y libavahi-compat-libdnssd-dev
COPY . .
CMD ["node", "mdns-publisher.js"]