# NexusCore Developer Onboarding
*Start Building Intelligent Agents*

## Welcome to NexusCore! 🎉

This guide will help you get started with building and deploying AI agents on the NexusCore platform. In less than 10 minutes, you'll have your first agent up and running.

## Table of Contents
1. [Quick Start](#quick-start)
2. [Core Concepts](#core-concepts)
3. [Installation](#installation)
4. [Your First Agent](#your-first-agent)
5. [Testing Locally](#testing-locally)
6. [Deployment](#deployment)
7. [Next Steps](#next-steps)
8. [Getting Help](#getting-help)

## Quick Start

### Prerequisites
- Python 3.8+ or Node.js 16+
- Git
- A NexusCore account (sign up at [nexuscore.ai](https://nexuscore.ai))

### 1. Install the NexusCore CLI

```bash
# For Python users
pip install nexuscore

# For Node.js users
npm install -g @nexuscore/cli
```

### 2. Login to your account

```bash
nexuscore login
```

### 3. Create your first agent

```bash
# Create a new agent
nexuscore create my-first-agent

# Navigate to the agent directory
cd my-first-agent
```

### 4. Run your agent locally

```bash
# Start the development server
nexuscore dev
```

### 5. Deploy your agent

```bash
# Deploy to NexusCore
nexuscore deploy
```

Congratulations! Your agent is now live on NexusCore. 🚀

## Core Concepts

### 1. Agents
Agents are autonomous programs that can perceive their environment and take actions to achieve specific goals.

### 2. Skills
Reusable capabilities that agents can use, such as:
- Web search
- File I/O
- API integrations
- Data processing

### 3. Memory
Agents can maintain state and context using different types of memory:
- Short-term (conversation history)
- Long-term (vector databases, knowledge graphs)

### 4. Tools
Pre-built utilities that agents can use:
- Web browsers
- Code executors
- File managers

## Installation

### System Requirements
- 8GB RAM minimum (16GB recommended)
- 5GB free disk space
- Stable internet connection

### Detailed Installation

#### 1. Install Dependencies

**macOS/Linux:**
```bash
# Install Python and pip
sudo apt update && sudo apt install -y python3 python3-pip

# Or using Homebrew (macOS)
brew install python
```

**Windows:**
1. Download Python from [python.org](https://www.python.org/downloads/)
2. Make sure to check "Add Python to PATH" during installation

#### 2. Set Up Virtual Environment (Recommended)

```bash
# Create a virtual environment
python -m venv .venv

# Activate the virtual environment
# On Windows:
.venv\Scripts\activate
# On macOS/Linux:
source .venv/bin/activate
```

## Your First Agent

Let's create a simple weather agent that can fetch the current weather for a location.

### 1. Initialize a New Agent

```bash
nexuscore create weather-agent --template=basic
cd weather-agent
```

### 2. Edit the Agent Code

Open `agent.py` in your favorite editor and modify it:

```python
from nexuscore import Agent, tool
import requests

class WeatherAgent(Agent):
    def __init__(self):
        super().__init__("weather_agent")
        self.description = "I can fetch the current weather for any location."
    
    @tool
    async def get_weather(self, location: str) -> str:
        """Get the current weather for a location."""
        # In a real agent, you would use a weather API
        return f"The weather in {location} is 72°F and sunny."

# Create an instance of your agent
agent = WeatherAgent()
```

### 3. Test Your Agent

```bash
# Start the development server
nexuscore dev
```

In another terminal, test your agent:

```bash
nexuscore invoke weather_agent get_weather --location="New York"
```

## Testing Locally

### Unit Tests

Create a `test_agent.py` file:

```python
import pytest
from agent import WeatherAgent

@pytest.mark.asyncio
async def test_weather_agent():
    agent = WeatherAgent()
    response = await agent.get_weather("New York")
    assert "New York" in response
```

Run the tests:

```bash
pytest
```

### Integration Tests

Create a `tests/integration` directory and add test files for API integrations and external services.

## Deployment

### 1. Configure Your Agent

Create a `nexuscore.yaml` file:

```yaml
name: weather-agent
version: 0.1.0
description: A simple weather agent
runtime: python3.8

permissions:
  - internet
  - storage

environment:
  OPENAI_API_KEY: ${env:OPENAI_API_KEY}
```

### 2. Deploy to NexusCore

```bash
nexuscore deploy
```

### 3. Monitor Your Agent

```bash
# View logs
nexuscore logs

# Check status
nexuscore status
```

## Next Steps

### 1. Add More Skills
- Connect to external APIs
- Add natural language processing
- Implement memory and context

### 2. Publish to Marketplace

```bash
# Package your agent
nexuscore package

# Publish to NexusCore Marketplace
nexuscore publish
```

### 3. Join the Community
- [Discord](https://discord.gg/nexuscore)
- [GitHub Discussions](https://github.com/nexuscore/nexuscore/discussions)
- [Documentation](https://docs.nexuscore.ai)

## Getting Help

### Documentation
- [API Reference](https://docs.nexuscore.ai/api)
- [Tutorials](https://docs.nexuscore.ai/tutorials)
- [Examples](https://github.com/nexuscore/examples)

### Support
- [Community Forum](https://github.com/nexuscore/nexuscore/discussions)
- Email: support@nexuscore.ai
- Twitter: [@nexuscore_ai](https://twitter.com/nexuscore_ai)

### Reporting Issues

Found a bug? Please file an issue on our [GitHub repository](https://github.com/nexuscore/nexuscore/issues).

---

Happy coding! 🚀

The NexusCore Team
