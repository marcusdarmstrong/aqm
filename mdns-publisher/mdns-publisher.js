import { exec } from 'node:child_process';
import { setInterval } from 'node:timers/promises';

const registrations = {};

for await (const ping of setInterval(30_000)) {
  const response = await fetch(`${process.env.TRAEFIK_API_HOST}/api/http/routers`)
  if (response.ok) {
  	const routers = await response.json();
  	const hosts = new Set(routers.map(({ rule }) => {
  	  const match = rule.match(/^Host\(`([a-z0-9_-]+local)`\)$/)
  	  if (match) {
		const [ full, host, ...rest ] = match;
  	  	if (!(host in registrations)) {
  	  	  const aborter = new AbortController();
  	  	  exec(`avahi-publish -R -a homebridge.home.local ${process.env.BINDING_IP_ADDRESS}`, { signal: aborter.signal });
  	  	  registrations[host] = aborter.abort;
  	    }	
  	  }
  	}));
  	Object.entries(registrations).forEach(([host, abort]) => {
  	  if (!hosts.has(host)) {
  	  	abort();
  	  	delete registrations[host];
  	  }
  	});
  }
}
