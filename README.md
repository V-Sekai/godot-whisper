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

## How to install

Go to a github release, copy paste the addons folder to the demo folder. Restart godot editor.

## Requirements

- Sconstruct(if you want to build locally)
- A language model, can be downloaded in godot editor.


## CaptureStreamToText

`CaptureStreamToText` - node that uses SpeechToText and runs transcribe function every 1 seconds.

Also has helper to download language model. Exposes all properties from singleton and configures them.

## Separate thread

The transcribe runs on a separate thread, as to not block main thread.

## Language Model

Go to a `CaptureStreamToText` node, select a Language Model to Download and click Download. You might have to alt tab editor or restart for asset to appear. Then, select `language_model` property.

## SpeechToText

`SpeechToText` Singleton has some configurable properties for transcribing:

```
float entropy_threshold [default: 2.8]
float freq_thold [default: 200.0]
int language [default: 1]
WhisperResource language_model
int max_tokens [default: 32]
int n_threads [default: 4]
bool speed_up [default: false]
bool translate [default: false]
bool use_gpu [default: true]
float vad_thold [default: 0.3]
```

And a function to add audio data:

```
add_audio_buffer(buffer: PackedVector2Array)
```

As well as a signal for when there is new data transcribed:

```
update_transcribed_msgs(process_time_ms: int, transcribed_msgs: Array)
```

In order to start the transcribing, use the start_listen and stop_listen methods.

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
