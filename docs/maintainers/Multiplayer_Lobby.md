# Multiplayer lobby

- RAM requirements: ~1 GB (can be potentially reduced if we were to track down all un-indexed sqlite queries)
- CPU requirements: 1 core (preferrably dedicated to ensure low latency)
- SSD requirements: up to 4 Gb (depending on log and database size over time)

Exposed to public as `beholder.vcmi.eu:3031` or `lobby.vcmi.eu:3031`

- Start: `cd /home/vcmilobby/build/bin && ./vcmilobby &`
- Stop: `killall -9 vcmilobby`
- Examine database (can be done live): `sqlite3 ~/.local/share/vcmi/vcmiLobby.db`
- Examine log file: `nano ~/.cache/vcmi/VCMI_Lobby_log.txt`

## TODO

- Update lobby setup to allow running it as non-root
- Set up PPA with lobby builds and install lobby from PPA instead of building from sources
- Migrate lobby to new domain name (`lobby.vcmi.eu:3031`). Needs adjustment to automatically move existing login information
- (long term) expand lobby functionality, including vcmiserver hosting for cheat-proof MP

## Setup

```sh
git clone https://github.com/vcmi/vcmi.git
mkdir build
cmake -DENABLE_EDITOR=OFF -DENABLE_CLIENT=OFF -DENABLE_SERVER=OFF -DENABLE_LAUNCHER=OFF -DENABLE_TEST=OFF -DENABLE_LOBBY=ON -DENABLE_MINIMAL_LIB=ON -DENABLE_PCH=OFF -DENABLE_STATIC_LIBS=ON ../vcmi`
make
```

Note: rebuild may take up to 15 minutes. During updates you might want to rebuild first, without shutting down server and then - quickly restart server
Note: server has configured swap file specifically to allow building vcmilobby locally. System memory usage was slightly over 1 Gb during build.

### Upgrade

```sh
cd /home/vcmilobby/vcmi && git pull --ff-only
cd /home/vcmilobby/build && make
killall -9 vcmilobby
cd /home/vcmilobby/build/bin && ./vcmilobby &
```

#### Migration

- setup binary on new server
- stop old server
- transfer database at `~/.local/share/vcmi/vcmiLobby.db` to new server
- adjust domain name to point to new server. WARNING: clients store login information based on DNS name. If new server has new DNS name, some code adjustments will be needed
- start new server
