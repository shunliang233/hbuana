# DatManager Data Structure Documentation

## Overview
The DatManager class handles binary data processing for AHCAL (Analog Hadronic Calorimeter) detector readout. It processes raw binary files containing event data from SPIROC (SiPM Integrated ReadOut Chip) and converts them to ROOT format.

## Data Flow Architecture
```
Binary File → Event Bags → SPIROC Bags → Chip Buffers → ROOT Tree
```

## Binary Data Format Structure

### 1. Event Bag Structure
Event bags are the top-level data containers in the binary stream.

- **Start Marker**: `0xfb 0xee 0xfb 0xee` (4 bytes)
- **End Marker**: `0xfe 0xdd 0xfe 0xdd` (4 bytes)
- **Cherenkov Counter**: Last 4 bytes before end marker
- **Data**: Contains multiple SPIROC bags between markers

### 2. SPIROC Bag Structure
SPIROC bags contain data from individual detector layers.

- **Start Marker**: `0xfa 0x5a 0xfa 0x5a` (4 bytes)
- **End Marker**: `0xfe 0xee 0xfe 0xee` (4 bytes)
- **Layer Information**: Follows end marker
  - Layer FF marker: `0xff` (1 byte)
  - Layer ID: 0-39 (1 byte)
- **Cycle ID**: 32-bit value (bytes 2-3 after start marker)
- **Trigger ID**: 16-bit value (byte 4 after start marker)
- **Minimum Size**: 74 bytes  
    - This includes the 4-byte start marker, 4-byte end marker, 2 bytes for layer information, and at least 64 bytes of chip data (corresponding to one memory unit of 32 words × 2 bytes/word). Actual size may be larger if more chip data is present.
    - 
- **Empty Layer Handling**: If no chip data is present for a layer, the SPIROC bag will contain only the start marker, end marker, and layer information (10 bytes total). The DecodeAEvent() function will skip processing for that layer, and no ROOT tree entries will be generated for channels in that layer.
- **Size Validation**: Must be even-sized

### 3. Chip Buffer Structure
Individual chip data within SPIROC bags.

- **Channel Data**: 36 channels × 2 data types (TDC + ADC) = 72 data words + metadata
- **Chip ID**: 1-9 (stored as last element, used as 0-8 internally)
- **BCID**: Bunch Crossing ID (second to last element) ?
- **Memory Units**: Data organized in groups of 73 words (72 data + 1 metadata)

## Channel Data Format (16-bit per channel)

### Auto Gain Mode (b_auto_gain = true)
```
TDC Data (Channel i, i = 0-35):
Bit 15-13: Reserved
Bit 12:    Hit Tag (1 = hit detected)
Bit 11-0:  TDC Value (0-4095)

ADC Data (Channel i + 36, i = 0-35):
Bit 15-14: Reserved  
Bit 13:    Gain Tag (1 = high gain, 0 = low gain)
Bit 12:    Reserved
Bit 11-0:  ADC Value (0-4095)
```

### Manual Mode (b_auto_gain = false)
```
High Gain Data (Channel i, i = 0-35):
Bit 15-13: Reserved
Bit 13:    Gain Tag TDC
Bit 12:    Reserved
Bit 11-0:  High Gain ADC Value

Low Gain Data (Channel i + 36, i = 0-35):
Bit 15-13: Reserved
Bit 12:    Reserved
Bit 11-0:  Low Gain ADC Value
```

## ROOT Tree Output Structure

### Branch Definitions
```cpp
Int_t    Run_Num;           // Run number extracted from filename
UInt_t   Event_Time;        // Event timestamp (30 bits from cherenkov counter)
Int_t    CycleID;           // Cycle identifier from SPIROC bag
Long64_t TriggerID;         // Trigger identifier (with loop correction)
vector<Long64_t> CellID;    // Encoded cell identifiers
vector<Int_t>    BCID;      // Bunch crossing identifiers ?
vector<Int_t>    HitTag;    // Hit detection flags (0/1)
vector<Int_t>    GainTag;   // Gain selection flags (-1/0/1)
vector<Int_t>    HG_Charge; // High gain charge measurements
vector<Int_t>    LG_Charge; // Low gain charge measurements
vector<Int_t>    Hit_Time;  // Hit timing information
vector<Int_t>    GainTag_TDC; // TDC gain flags
vector<Int_t>    Cherenkov; // Cherenkov detector signals
```

### Cell ID Encoding
```
CellID = layer_id × 10^5 + chip_id × 10^4 + memo_id × 10^2 + channel_id
```

Where:
- **layer_id**: Detector layer (0-39)
- **chip_id**: SPIROC chip within layer (0-8, stored as 1-9 in data)
- **memo_id**: Memory/readout unit ID (0-based)
- **channel_id**: Channel within chip (0-35, inverted: 35-channel_index)

## Constants and Configuration

### Hardware Limits
```cpp
static const int Layer_No = 40;     // Maximum detector layers
static const int chip_No = 9;       // Maximum chips per layer
static const int channel_No = 36;   // Channels per chip
static const int channel_FEE = 73;  // Channel data + metadata per memory unit
```

### Data Processing Constants
- **Maximum Layer ID**: 39
- **Maximum Chip ID**: 9 (stored as 1-9, used as 0-8)
- **Channel Inversion**: Channel ID = 35 - channel_index (for 36 channels: 0-35)
- **Memory Unit Size**: 73 words (72 data words: 36 TDC + 36 ADC + 1 control word)
- **Data Organization**: Each channel produces 2 data words (TDC and ADC)

**Note**: The position arrays `_Pos_X` and `_Pos_Y` in Global.h are defined for only 36 channels, which appears to be a legacy configuration or partial channel mapping.

## Cherenkov Data Format
The cherenkov counter is a 32-bit value with the following structure:

```
Bit 31:    Cherenkov Detector 1 Signal
Bit 30:    Cherenkov Detector 2 Signal  
Bit 29-0:  Event Timestamp (30 bits)
```

Extraction:
```cpp
cherenkov[0] = (cherenkov_counter & 0x80000000) / 0x80000000; // Detector 1
cherenkov[1] = (cherenkov_counter & 0x40000000) / 0x40000000; // Detector 2
Event_Time = (cherenkov_counter & 0x3fffffff);               // Timestamp
```

## Error Handling and Validation

### Validation Checks
1. **Bag Size Validation**: 
   - SPIROC bags must be even-sized and ≥74 bytes
   - Event bags must contain valid start/end markers

2. **Layer ID Range**: Must be 0-39
   ```cpp
   if(buffer < 0 || buffer > 39) {
       cout << "abnormal layer " << hex << buffer << endl;
       return 0;
   }
   ```

3. **Chip ID Range**: Must be 1-9
   ```cpp
   if (buffer_v[i] < 1 || buffer_v[i] > 9) continue;
   ```

4. **Marker Validation**: All start/end markers must match expected values
   - Event: `0xfb 0xee 0xfb 0xee` / `0xfe 0xdd 0xfe 0xdd`
   - SPIROC: `0xfa 0x5a 0xfa 0x5a` / `0xfe 0xee 0xfe 0xee`
   - Chip: `0xfa5a 0xfa5a` / `0xfeee 0xfeee`

5. **Memory Unit Validation**: Chip data size must be (n×73 + 1) words
   ```cpp
   if((size % 73) != 1) {
       cout << "wrong chip size " << chip_v.size() << endl;
       return 0;
   }
   ```

### Error Recovery
- **Invalid bags**: Discarded with error messages
- **Abnormal chip buffers**: Cleared and logged with counter
- **Trigger ID mismatches**: Flagged and handled with loop detection
- **Size mismatches**: Logged and data cleared
- **Missing markers**: Data rejected and processing continues

### Loop Detection
```cpp
if(last_trigID - pre_trigID > 40000) {
    cout << "Loop " << pre_trigID << " " << last_trigID << endl;
    Loop_No++;
}
```

## Data Processing Pipeline

1. **CatchEventBag()**: Extracts event bags from binary stream
   - Searches for start marker `0xfb 0xee 0xfb 0xee`
   - Reads until end marker `0xfe 0xdd 0xfe 0xdd`
   - Extracts cherenkov counter from last 8 bytes

2. **CatchSPIROCBag()**: Extracts SPIROC data from event bags
   - Finds SPIROC markers within event bag
   - Validates layer information
   - Extracts cycle and trigger IDs
   - Converts byte pairs to 16-bit words

3. **FillChipBuffer()**: Organizes chip data by layer and chip ID
   - Validates chip packet markers
   - Distributes data to appropriate chip buffers
   - Handles multiple memory units per chip

4. **DecodeAEvent()**: Converts raw chip data to physics quantities
   - Processes 36 channels per memory unit (72 data words total)
   - Applies gain mode logic
   - Extracts timing and charge information
   - Populates ROOT tree branches

5. **ROOT Output**: Fills tree branches and writes to file
   - Applies cell ID encoding
   - Handles auto/manual gain modes
   - Includes cherenkov coincidence detection

## Statistics and Monitoring

The system tracks:
- **Event counts**: Total events processed
- **Cherenkov events**: Individual detector and coincidence counts
- **Abnormal events**: Trigger ID mismatches and other errors
- **Chip buffer errors**: Malformed chip data packets
- **Loop events**: Trigger ID rollover detection

This structure allows for efficient processing of multi-layer detector data with robust error handling and flexible gain mode support.