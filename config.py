def can_build(env, platform):
    return platform != "web" and platform != "android"


def configure(env):
    pass


def get_doc_classes():
    return [
        "SpeechToText",
        "SpeechToTextProcessor",
        "PlaybackStats",
    ]


def get_doc_path():
    return "doc_classes"
