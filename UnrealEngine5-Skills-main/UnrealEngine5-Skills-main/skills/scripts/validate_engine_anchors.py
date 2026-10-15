#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


SUPPORTED_MINOR_VERSIONS = {6, 7, 8}


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate UE5.6-UE5.8 engine API anchors used by this skill pack."
    )
    parser.add_argument(
        "--engine-root",
        required=True,
        type=Path,
        help="Engine install root, Engine directory, or Engine/Source directory.",
    )
    return parser.parse_args(argv)


def normalize_engine_dir(path_like: Path) -> Path:
    path = path_like.expanduser().resolve()
    if path.name.lower() == "source" and path.parent.name.lower() == "engine":
        return path.parent
    if path.name.lower() == "engine":
        return path
    return path / "Engine"


def read_engine_version(engine_dir: Path) -> tuple[int, int, int]:
    build_version = engine_dir / "Build" / "Build.version"
    if not build_version.exists():
        raise ValueError(f"Build.version not found: {build_version}")
    data = json.loads(build_version.read_text(encoding="utf-8-sig"))
    version = (
        int(data["MajorVersion"]),
        int(data["MinorVersion"]),
        int(data.get("PatchVersion", 0)),
    )
    if version[0] != 5 or version[1] not in SUPPORTED_MINOR_VERSIONS:
        raise ValueError(
            f"Unsupported engine version {version[0]}.{version[1]}.{version[2]}; "
            "expected UE5.6, UE5.7, or UE5.8"
        )
    return version


def require_tokens(engine_dir: Path, relative_path: str, tokens: tuple[str, ...]) -> list[str]:
    path = engine_dir / relative_path
    if not path.exists():
        return [f"missing header: {path}"]
    text = path.read_text(encoding="utf-8", errors="ignore")
    return [f"{path}: missing API anchor '{token}'" for token in tokens if token not in text]


def validate(engine_dir: Path, minor_version: int) -> list[str]:
    checks: list[tuple[str, tuple[str, ...]]] = [
        (
            "Source/Runtime/Engine/Classes/Kismet/GameplayStatics.h",
            ("AsyncSaveGameToSlot", "AsyncLoadGameFromSlot"),
        ),
        (
            "Source/Runtime/UMG/Public/Blueprint/WidgetBlueprintLibrary.h",
            ("SetInputMode_UIOnlyEx", "SetInputMode_GameAndUIEx", "SetInputMode_GameOnly"),
        ),
        (
            "Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h",
            ("GetAssetsByPath", "GetDependencies", "GetReferencers"),
        ),
        (
            "Source/Runtime/Engine/Classes/Engine/World.h",
            ("LineTraceSingleByChannel", "SweepSingleByChannel"),
        ),
        (
            "Source/Developer/DeveloperToolSettings/Classes/Settings/ProjectPackagingSettings.h",
            ("UProjectPackagingSettings",),
        ),
        (
            "Plugins/PCG/Source/PCG/Public/PCGComponent.h",
            (
                "EPCGComponentGenerationTrigger",
                "GenerateLocal",
                "bOverrideGenerationRadii",
                "SchedulingPolicyClass",
            ),
        ),
        (
            "Plugins/PCG/Source/PCG/Public/Elements/Grammar/PCGSubdivisionBase.h",
            ("GrammarSelection",),
        ),
    ]

    if minor_version >= 8:
        checks.append(
            (
                "Plugins/PCG/Source/PCG/Public/Subsystems/PCGSubsystem.h",
                (
                    "RefreshRuntimeGenExecutionSource",
                    "RefreshAllRuntimeGenExecutionSources",
                    "DirtyRuntimeGenExecutionSources",
                    'UE_DEPRECATED(5.8, "Use RefreshRuntimeGenExecutionSource instead")',
                    'UE_DEPRECATED(5.8, "Use RefreshAllRuntimeGenExecutionSources instead")',
                ),
            )
        )
    else:
        subsystem_path = (
            "Plugins/PCG/Source/PCG/Public/Subsystems/PCGSubsystem.h"
            if minor_version >= 7
            else "Plugins/PCG/Source/PCG/Public/PCGSubsystem.h"
        )
        checks.append(
            (
                subsystem_path,
                ("RefreshRuntimeGenComponent", "RefreshAllRuntimeGenComponents"),
            )
        )

    errors: list[str] = []
    for relative_path, tokens in checks:
        errors.extend(require_tokens(engine_dir, relative_path, tokens))
    return errors


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    engine_dir = normalize_engine_dir(args.engine_root)
    try:
        version = read_engine_version(engine_dir)
    except (KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
        print(f"Engine anchor validation FAILED: {exc}")
        return 2

    errors = validate(engine_dir, version[1])
    version_text = ".".join(str(part) for part in version)
    if errors:
        print(f"Engine anchor validation FAILED for UE{version_text}:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Engine anchor validation OK for UE{version_text}")
    print(f"- engine: {engine_dir}")
    print("- stable gameplay, UI, asset, trace, packaging, and PCG anchors found")
    print("- PCG scheduler branch matches the detected engine version")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
