# Godot Whisper

<p align="center">
	<a href="https://github.com/V-Sekai/godot-whisper/actions/workflows/runner.yml">
        <img src="https://github.com/V-Sekai/godot-whisper/actions/workflows/runner.yml/badge.svg?branch=main"
            alt="chat on Discord"></a>
    <a href="https://github.com/ggerganov/whisper.cpp" alt="Whisper CPP">
        <img src="https://img.shields.io/badge/WhisperCPP-v1.5.1-%23478cbf?logoColor=white" /></a>
    <a href="https://github.com/godotengine/godot-cpp" alt="Godot Version">
        <img src="https://img.shields.io/badge/Godot-v4.1-%23478cbf?logo=godot-engine&logoColor=white" /></a>
    <a href="https://github.com/V-Sekai/godot-whisper/graphs/contributors" alt="Contributors">
        <img src="https://img.shields.io/github/contributors/V-Sekai/godot-whisper" /></a>
    <a href="https://github.com/V-Sekai/godot-whisper/pulse" alt="Activity">
        <img src="https://img.shields.io/github/commit-activity/m/V-Sekai/godot-whisper" /></a>
    <a href="https://discord.gg/H3s3PD49XC">
        <img src="https://img.shields.io/discord/1138836561102897172?logo=discord"
            alt="Chat on Discord"></a>
</p>

<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-2-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->

<p align="center">
<img src="whisper_cpp.gif"/>
</p>

## Features

- Realtime audio transcribe.
- Audio transcribe with recorded audio.
- Runs on separate thread.
- Metal for Apple devices.
- OpenCL for rest.

## How to install

Go to a github release, copy paste the addons folder to the demo folder. Restart godot editor.

## Requirements

- Sconstruct(if you want to build locally)
- A language model, can be downloaded in godot editor.

## AudioStreamToText

`AudioStreamToText` - this node can be used in editor to check transcribing. Simply add a WAV audio source and click start_transcribe button.

Normal times for this, using tiny.en model are about 0.3s. This only does transcribing.

## CaptureStreamToText

This runs also resampling on the audio(in case mix rate is not exactly 16000 it will process the audio to 16000). Then it runs every transcribe_interval transcribe function.

## Initial Prompt

For Chinese, if you want to select between Traditional and Simplified, you need to provide an initial prompt with the one you want, and then the model should keep that same one going. See [Whisper Discussion #277](https://github.com/openai/whisper/discussions/277).

Also, if you have problems with punctuation, you can give it an initial prompt with punctuation. See [Whisper Discussion #194](https://github.com/openai/whisper/discussions/194).

## Language Model

Go to any `StreamToText` node, select a Language Model to Download and click Download. You might have to alt tab editor or restart for asset to appear. Then, select `language_model` property.

## Global settings

Go to Project -> Project Settings -> General -> Audio -> Input (Check Advance Settings).

You will see a bunch of settings there.

Also, as doing microphone transcribing requires the data to be at a 16000 sampling rate, you can change the audio driver mix rate to 16000: `audio/driver/mix_rate`. This way the resampling won't need to do any work, winning you some valuable 50-100ms for larger audio, but at the price of audio quality.

## How to build

```
scons target=template_release generate_bindings=no arch=universal precision=single
rm -rf demo/addons
cp -rf bin/addons demo/addons
```

## Contributors ✨

Thanks goes to these wonderful people ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Ughuuu"><img src="https://avatars.githubusercontent.com/u/2369380?v=4?s=100" width="100px;" alt="Dragos Daian"/><br /><sub><b>Dragos Daian</b></sub></a><br /><a href="https://github.com/V-Sekai/v-sekai.whisper/commits?author=Ughuuu" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://chibifire.com"><img src="https://avatars.githubusercontent.com/u/32321?v=4?s=100" width="100px;" alt="K. S. Ernest (iFire) Lee"/><br /><sub><b>K. S. Ernest (iFire) Lee</b></sub></a><br /><a href="https://github.com/V-Sekai/v-sekai.whisper/commits?author=fire" title="Code">💻</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) specification. Contributions of any kind welcome!
