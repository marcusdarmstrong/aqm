import { spawn } from 'node:child_process';
import { setInterval } from 'node:timers/promises';

const registrations = {};
const loop = async () => {
  const response = await fetch(`${process.env.TRAEFIK_API_HOST}/api/http/routers`)
  if (response.ok) {
  	const routers = await response.json();
  	const hosts = new Set()
  	routers.forEach(({ rule }) => {
      const hostRule = rule.match(/Host\(([a-z0-9_\.`, \t-]+)\)/);
      if (hostRule === null) {
        return;
      }
      const [ _, hostSet, ...rest ] = hostRule;
      for (const match of hostSet.matchAll(/`([a-z0-9\._-]+\.local)`/g)) {
		    const [ m, host ] = match;
		    hosts.add(host);
  	  	if (!(host in registrations)) {
  	  	  const aborter = new AbortController();
  	  	  spawn('avahi-publish', ['-R', '-a', `${host}`, `${process.env.BINDING_IP_ADDRESS}`], { signal: aborter.signal, stdio: 'inherit' });
  	  	  registrations[host] = () => aborter.abort();
  	      console.log(`Published ${host}`);
  	    }
      }
  	});
  	Object.entries(registrations).forEach(([host, abort]) => {
  	  if (!hosts.has(host)) {
  	  	abort();
  	  	delete registrations[host];
  	  }
  	});
  }
}

await loop();
for await (const ping of setInterval(parseInt(process.env.POLL_INTERVAL, 10))) {
  await loop();
}
