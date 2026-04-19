# Extreme Racing Machines

Requires [Egg v2](https://github.com/aksommerville/egg2) to build.

For Gamedev.js Jam 2026, theme "MACHINES".

Orthographic racing, hopefully along the lines of SNES Micro Machines.

## TODO

- Important and urgent.
- [x] Structured races.
- [x] CPU driver.
- [x] When cars bonk each and travelling the same direction, there can be a ridiculous amount of force. Tone it down.
- [x] I think tone down the wall penalty.
- [ ] CPU players are getting stuck sometimes, esp in south_sea. Can we add some kind of mitigation? Similar: Can we prevent them from grinding into walls?
- - Try better comprehension of the line connecting plan points, rather than just "steer toward the next point".
- - That might also give us a means of determining rank in real time.
- [ ] Even 5-meter streets feel pretty narrow. Drop some of the 3s and add some 7s.
- - Redesign the world map altogether, now that we understand the rules better.
- [ ] south_sea race is dreadfully boring. Will need some more twists n turns.
- [ ] If we're going to let boats reach the map's edge, then the edges must participate in physics. But I think it's better to put a wall around the edge.
- - For that matter, let's put a *thick* wall around the edge, so you never see the camera clamp.
- [ ] Tune vehicle configs.
- [x] Boat and chopper.
- [x] Overpass and bridge. If it proves too hairy, we can do moveable bridges instead. Open when you pick the boat, close otherwise, easy. ...XXX too much trouble.
- [ ] Music.
- [ ] Incidental sound effects.
- [ ] Scorekeeping.
- [ ] Show time, speed, etc in a overlay.
- [ ] Big overlay with lap time, or the finish message.
- [x] Count down to start race.
- [ ] Need a lot more variety of architecture, not just for aesthetics, but so the player can remember where he is.
- [ ] Different appearance for CPU player.
- [x] Move more vehicle config out to the sprite resource, and tune all the vehicles appropriately.

- Stretch goals.
- [ ] Generalize count of CPU drivers in a race.
- [ ] Show current rank. Not as simple as it sounds.
- [ ] Boat: Animated motor.
- [ ] Boat: visible wake. Maybe trailing long behind the sprite, so not part of the sprite itself.
- [ ] High and low gear? I guess it would just set a lower top speed, in low gear. Might be helpful in narrow streets?
- [ ] Motor and screech effects, maybe with synth events instead of sounds?
- [ ] Modals.
- [ ] 2-player mode. Or why not more?
- [x] Non-racing challenges. Push a thing, demolish a car, weave around the cones, race with gas stuck on. ...XXX too much trouble.
- [x] Free play mode. Side quests, combat, dialogue? ...XXX too much trouble.
