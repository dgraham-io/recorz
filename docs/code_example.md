"Root prototype"
ProtoObject := Object clone.     "or whatever your root is"

"Create a new prototype by cloning"
PointProto := ProtoObject clone.

"Add slots (data or assignable) – all via messages"
PointProto addSlot: #x value: 0.           "constant slot"
PointProto addSlot: #y <- 0.               "assignable slot (if your runtime supports <- )"
PointProto addSlot: #color value: 'red'.

"Add methods via blocks"
PointProto addMethod: #distanceToOrigin 
           do: [ :self | ((self x * self x) + (self y * self y)) sqrt ].

"Clone and use"
p := PointProto clone.
p x: 3; y: 4.          "cascade"
p distanceToOrigin print.