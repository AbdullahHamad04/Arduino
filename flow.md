## Flowchart of robot behaviour

```mermaid
flowchart TD

A[Start] --> B[Initialize Arduino, sensors and motors]
B --> C[Read IR sensors]

C -->|Obstacle detected| D[Stop motors and avoid]
D --> C

C -->|No obstacle| E[Measure distance with ultrasonic]

E -->|Distance not valid| F[Sweep servo to search for target]
F --> C

E -->|Distance valid| G[Compute error = D - desired]

G -->|error > deadband| H[Move forward faster]
G -->|error < -deadband| I[Slow down or short reverse]
G -->|error within deadband| J[Keep speed or stop]

H --> C
I --> C
J --> C
