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

<p align="center">
<img src="whisper_cpp.gif"/>
</p>

<!-- ALL-CONTRIBUTORS-BADGE:START - Do not remove or modify this section -->
[![All Contributors](https://img.shields.io/badge/all_contributors-2-orange.svg?style=flat-square)](#contributors-)
<!-- ALL-CONTRIBUTORS-BADGE:END -->

## How to install

Go to a github release, copy paste the addons folder to the demo folder. Restart godot editor.

## SpeechToText

`SpeechToText` Node has a `transcribe` which gets a buffer that it transcribes.

## CaptureStreamToText

`CaptureStreamToText` - extends SpeechToText and runs transcribe function every 5 seconds.

## Main thread

The transcribe can block the main thread. It should run in about 0.5 seconds every 5 seconds, but check for yourself.

## Language Model

Go to a `CaptureStreamToText` node, select a Language Model to Download and click Download. You might have to alt tab editor or restart for asset to appear. Then, select `language_model` property.

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
