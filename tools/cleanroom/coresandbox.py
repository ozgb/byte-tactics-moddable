"""Run an untrusted core in a microVM of the clean-room image: one shared directory, no network.

    with Sandbox(image, share) as box:
        subprocess.run([*box.prefix(), "/path/in/share/core", ...])

The machine has no network device and one mount, `share`, read-write at the same absolute path
inside, so a path means the same on both sides. The share follows the mount rules of launch.py: it
is not, does not contain and does not lie inside this repository (except under build/scratch), and
overlaps no forbidden home path. A refusal raises SandboxRefused before or instead of running anything.
"""

import json
import os
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import launch

ELF_MAGIC = b"\x7fELF"
ELFCLASS64 = 2
ELFDATA2LSB = 1
ET_EXEC = 2
ET_DYN = 3
EM_X86_64 = 62
NET_FLAGS = (
    "--net", "--dns", "--guest-subnet", "--network", "--allow-cidr", "--deny-cidr", "--allow-udp", "--allow-host",
    "--allow-host-pattern", "--credential", "--outbound-localhost-only", "--block-private", "--allow-private",
    "--use-host-proxy", "--port", "-p", "--expose-socket", "--mount-socket", "--docker-socket", "--net-backend",
)
GRANT_FLAGS = (*launch.FORBIDDEN_FLAGS, "--secret-env", "--secret-file", "--secret-keychain", "--gpu", "--cuda", "--disk")


class SandboxRefused(Exception):
    pass


def is_core_executable(path):
    """Return None when the file is an x86_64 Linux ELF executable, else the reason it is not."""
    try:
        with open(path, "rb") as f:
            head = f.read(20)
    except OSError as err:
        return f"cannot read {path}: {err.strerror}"
    if len(head) < 20 or head[:4] != ELF_MAGIC:
        return "not an ELF file"
    if head[4] != ELFCLASS64:
        return "not a 64-bit ELF file"
    if head[5] != ELFDATA2LSB:
        return "not a little-endian ELF file"
    kind, machine = struct.unpack_from("<HH", head, 16)
    if machine != EM_X86_64:
        return f"not an x86_64 ELF file (e_machine {machine})"
    if kind not in (ET_EXEC, ET_DYN):
        return f"not an executable (e_type {kind})"
    return None


def create_argv(name, image, share, smolvm=launch.SMOLVM, cpus=4, mem=4096):
    """Return the `smolvm machine create` argv of a core sandbox: the one share, at its own path."""
    return [
        str(smolvm), "machine", "create", "--name", name, "--from", str(image),
        "--cpus", str(cpus), "--mem", str(mem),
        "-v", f"{share}:{share}",
    ]


def check_argv(argv, share):
    """Raise SandboxRefused unless a create argv has no network or grant flag and exactly the one share mount."""
    for flag in (*GRANT_FLAGS, *NET_FLAGS):
        if flag in argv:
            raise SandboxRefused(f"forbidden flag {flag}")
    mounts = [argv[i + 1] for i, a in enumerate(argv) if a in ("-v", "--volume")]
    if mounts != [f"{share}:{share}"]:
        raise SandboxRefused(f"unexpected mounts: {mounts}")


class Sandbox:
    """A started machine of the clean-room image with `share` mounted and no network; deleted on exit."""

    def __init__(self, image, share, smolvm=launch.SMOLVM, runner=subprocess.run):
        self.image = Path(image)
        self.share = Path(share)
        self.smolvm = Path(smolvm)
        self.runner = runner
        self.name = f"core-{os.getpid()}-{os.urandom(3).hex()}"
        self.created = False

    def run(self, *args):
        return self.runner([str(self.smolvm), *args], capture_output=True, text=True)

    def check_share(self):
        if ":" in str(self.share):
            raise SandboxRefused(f"share {self.share} has a colon in its path")
        home = Path.home()
        roots = launch.repo_roots()
        config_dir = launch.claude_config_dir(os.environ)
        try:
            share = launch.check_mount(self.share, "share", roots, home / ".nohome", config_dir=config_dir)
            for bad in launch.forbidden_paths(home, config_dir):
                if bad != home / "source" / "repos" and (launch.inside(share, bad) or launch.inside(bad, share)):
                    raise launch.Refusal(f"share {share} overlaps {bad}")
            repos = home / "source" / "repos"
            if launch.inside(share, repos) and not any(launch.inside(share, r / "build" / "scratch") for r in roots):
                raise launch.Refusal(f"share {share} is inside {repos}, outside build/scratch of a worktree")
            if launch.inside(repos, share):
                raise launch.Refusal(f"share {share} contains {repos}")
        except launch.Refusal as err:
            raise SandboxRefused(str(err)) from err
        return share

    def has_network(self):
        listing = self.run("machine", "ls", "--json").stdout
        try:
            machines = json.loads(listing)
        except ValueError as err:
            raise SandboxRefused(f"cannot read the machine list: {err}") from err
        return any(m.get("name") == self.name and m.get("network") for m in machines)

    def __enter__(self):
        share = self.check_share()
        self.share = share
        if not self.image.is_file():
            raise SandboxRefused(f"image {self.image} does not exist")
        argv = create_argv(self.name, self.image, share, self.smolvm)
        check_argv(argv, share)
        try:
            created = self.runner(argv, capture_output=True, text=True)
            self.created = True
            if created.returncode:
                raise SandboxRefused(f"machine create failed: {(created.stderr or '').strip()}")
            self.run("machine", "update", "--name", self.name, "--no-net")
            if self.has_network():
                raise SandboxRefused(f"machine {self.name} still has a network after --no-net")
            started = self.run("machine", "start", "--name", self.name)
            if started.returncode:
                raise SandboxRefused(f"machine start failed: {(started.stderr or '').strip()}")
        except BaseException:
            self.close()
            raise
        return self

    def prefix(self):
        """Return the argv prefix that runs a command inside the machine."""
        return [str(self.smolvm), "machine", "exec", "--name", self.name, "--"]

    def close(self):
        if self.created:
            self.created = False
            self.run("machine", "stop", "--name", self.name)
            self.run("machine", "delete", "--name", self.name, "--force")

    def __exit__(self, *exc):
        self.close()
        return False
