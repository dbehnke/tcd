# TCD Architecture & Data Flow

This document details the internal data flow of the Transcoder (TCD) and the injection point of the Automatic Gain Control (AGC).

## High-Level Data Flow

`tcd` operates by receiving packets from a reflector (e.g., `urfd` or `xlxd`), decoding them to PCM Audio, and then re-encoding them to the target format(s) needed by other modules.

```mermaid
graph TD
    Reflector["Reflector (urfd)"] -->|Network Packet| TCD_RX[TCD Receiver]
    TCD_RX -->|Queue Packet| Queues[Encoding Queues]
    
    Config["Configuration (tcd.ini)"] -->|AGCTargetLevel| Controller[Controller]
    
    subgraph "Transcoding Loop (The Matrix)"
    direction TB
        Queues -->|Pop Packet| Decoder["Decoder (Soft/Hard)"]
        Decoder -->|PCM Audio| PCM["Raw PCM 16-bit"]
        
        style PCM fill:#f96,stroke:#333,stroke-width:2px,color:#000
        
        PCM --> AGC_Block{AGC Enabled?}
        
        Controller -.->|Set Target| AGC_Process
        
        AGC_Block -- Yes --> AGC_Process["CAGC::Process<br/>(Normalize to Configured Target)"]
        AGC_Block -- No --> Gain_Process["Manual Gain<br/>(Fixed Multiplier)"]
        
        AGC_Process --> Encoder["Encoder (Soft/Hard)"]
        Gain_Process --> Encoder
        
        Encoder -->|Encoded Packet| TCD_TX[TCD Sender]
    end
    
    TCD_TX -->|Network Packet| Reflector
```

## Detailed Processing Threads

The transcoding is handled by specific threads depending on the input format.

### 1. Hardware Transcoding (DV3000/DV3003)

Used for D-Star and DMR (if software AMBE is disabled).

```mermaid
sequenceDiagram
    participant Q as Input Queue
    participant H as Hardware (DV3000)
    participant C as Controller
    participant A as AGC
    participant D as Dest Queue

    Q->>H: Send AMBE Data (Encode Request)
    H-->>C: Return PKT_SPEECH (PCM Audio)
    C->>A: ProcessAGC(PCM)
    A-->>C: Normalized PCM
    C->>D: Route to various encoders
    
    Note over D: Encoders (YSF, P25, M17) use this<br/>normalized PCM for clean audio.
```

### 2. Software Transcoding (SWAMBE / IMBE / Codec2)

Used for DMR (Soft), P25, and M17.

```mermaid
graph LR
    Input[Input Packet] --> Decode{Decode}
    
    Decode -->|DMR| SWAMBE[md380_decode]
    Decode -->|P25| IMBE[p25_decode]
    Decode -->|M17| C2[codec2_decode]
    
    SWAMBE --> PCM[PCM Audio]
    IMBE --> PCM
    C2 --> PCM
    
    PCM --> AGC[CAGC::Process]
    
    AGC --> Encode{Encode Targets}
    
    Encode -->|DMR| SWAMBE_Enc[md380_encode]
    Encode -->|P25| IMBE_Enc[p25_encode]
    Encode -->|M17| C2_Enc[codec2_encode]
```

## Configuration

1. **Parses `tcd.ini`**: Reads `AGC=1` and `AGCTargetLevel` (float).
2. **Controller**: Calculates linear target from dBFS and updates AGC instances.

## AGC Algorithm

The AGC uses a generic Peak Envelope Follower approach:

1. **Detect**: Track absolute peak level of incoming PCM (Fast Attack, Slow Release).
2. **Calculate**: Compare peak to Target Level (-3dBFS).
3. **Apply**: Adjust gain smoothly to match Target.
4. **Limit**: Hard limitation prevents clipping/wrap-around.

This ensures that loud inputs (like hot DMR users) are attenuated, and quiet inputs are boosted, normalizing the audio for all bridged modes.
