auto cookie

getObservation() -> normalize cookie count that is returned; include parameters like cps, buildings owned, etc
add cookies baked all time;

make dt chosen by policy
discretize dt into log-spaced menu : 1 tick, 10, 100, ...

for each building: time until affordable -> into getObservation()
action: skip to next afordable purchase

policy -> requests huge dt -> internally, engine walks forward in small substeps until relevant event happens (e.g. golden cookie appears) and returns early
allow dt=0 for multiple building purchases?
