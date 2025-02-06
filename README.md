# Nerd-CLI

A Chatbot running inside of the Terminal using ollama API.  
**ONLY SUPPORTS WINDOWS**

# Installation

**Make sure you have ollama installed on your system and at least one model downloaded.**  
**Preferably the llama3.2:3b model**

### Requirements

- [ollama](https://ollama.com/)
- [llama3.2:3b](https://ollama.com/library/llama3.2:3b) (OPTIONAL FOR STANDARD MODEL)

### Installing llama3.2

```bash
ollama run llama3.2:3b
```

Download the binary from [Releases](https://github.com/aspect22/Nerd-CLI/releases) and extract it to a folder.  
After that run the binary from the terminal using the following command:

```bash
./nerd
```

# Usage

### Help command

```bash
$ ./nerd --help
---USAGE---
Arg1(Prompt): "Your question here"
Arg2(Model): "deepseek-r1:8b / llama3.2:3b (DEFAULT) / etc"
```

### Example

No model selection, default is llama3.2:3b

```bash
$ ./nerd "What is the capital of France?"
```

Model selection, any installed model

```bash
$ ./nerd "What does x86-64 mean?" "deepseek-r1:8b"
```

# To-Do

- [ ] No hardcoded default model
- [ ] Optimization (Probably)
- [ ] Config File

# Credits

No one cuz I made this myself lol.  
Jk i can't code credits to ChatGPT for helping me
