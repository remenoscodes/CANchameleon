# CAN Chameleon: A funny CAN Network Tool

## Introduction

Meet the CAN Chameleon, a dynamic, dual-purpose device designed for automotive diagnostics, education, and network simulation. With its unique ability to seamlessly switch between **Sniffing Mode** and **Injector Simulator Mode**, the CAN Chameleon embodies adaptability and versatility in CAN network interaction.

## Key Features

- **Dual Operating Modes**: Effortlessly transition between monitoring network traffic and simulating CAN nodes.
- **Efficient ID Management**: Utilizes a bitmap within a 150-byte limit to intelligently manage and filter CAN IDs across two selectable ranges.
- **Intuitive LED Indicators**:
  - **RED LED**: Lights up in Injector Simulator Mode, when the device actively introduces messages into the CAN network.
  - **BLUE LED**: Glows in Sniffing Mode, signifying the device's role in observing and analyzing network traffic.
  - **YELLOW LED**: Indicates the device is operational and has successfully initialized.
  - **GREEN LED**: Blinks with every message sent to the network, a sign of active simulation.
  - **BLUE LED (second one)**: Blinks upon receiving a message, ensuring you're informed of incoming data.
- **Flexible Range Selection**: Choose between two CAN ID ranges (0-1023 and 1024-2047) for focused analysis or simulation, enabled by user-friendly selection buttons.
- **Optional OLED Display**: Offers real-time insights into operational mode, active CAN ID range, and memory usage—perfect for in-depth analysis.

## Getting Started with CAN Chameleon

1. **Power Up**: Connect your CAN Chameleon to a power source and your CAN network. The YELLOW LED will indicate a successful initialization.
2. **Select Your Mode**: Use the OPERATION MODE button to switch between Sniffing and Injector Simulator Modes. Watch the RED and BLUE LEDs to identify your current mode.
3. **Choose Your Range**: Determine which CAN ID range you wish to engage with and use the RANGE GROUP SELECT buttons to narrow down your focus.
4. **Monitor or Simulate**: Begin monitoring the network or simulating traffic. Observe the GREEN and BLUE LEDs for real-time activity feedback.
5. **Deep Dive with the Display**: If your device is equipped with an OLED display, enjoy detailed monitoring of operational data and network statistics.

## Applications

The CAN Chameleon is suited for a variety of applications, from automotive repair shops needing detailed diagnostics to educators teaching CAN network fundamentals. Its versatile functionality also makes it an excellent tool for developers testing new CAN-based systems.

## Legend

- **RED LED**: "INJECTOR ACTIVE" – Broadcasting CAN messages.
- **BLUE LED**: "SNIFFING TRAFFIC" – Observing and logging.
- **YELLOW LED**: "SYSTEM READY" – All systems go.
- **GREEN LED**: "MESSAGE OUT" – Data sent to CAN.
- **BLUE LED (second one)**: "MESSAGE IN" – Data received from CAN.
- **RANGE SELECT BUTTONS**: "FOCUS ZONE" – Select your CAN ID range.
- **MODE SELECT BUTTON**: "MODE SHIFT" – Toggle between your operational roles.

---

The CAN Chameleon is your adaptable tool for navigating the complexities of CAN networks, designed to blend into any automotive environment with ease and precision. Whether you're sniffing out issues or injecting new data, the CAN Chameleon is the perfect companion for all your CAN network adventures.