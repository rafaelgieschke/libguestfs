(* Common utilities for OCaml tools in libguestfs.
 * Copyright (C) 2010-2015 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *)

val ( // ) : string -> string -> string
(** Concatenate directory and filename. *)

val ( +^ ) : int64 -> int64 -> int64
val ( -^ ) : int64 -> int64 -> int64
val ( *^ ) : int64 -> int64 -> int64
val ( /^ ) : int64 -> int64 -> int64
val ( &^ ) : int64 -> int64 -> int64
val ( ~^ ) : int64 -> int64
(** Various int64 operators. *)

val roundup64 : int64 -> int64 -> int64
val div_roundup64 : int64 -> int64 -> int64
val int_of_le32 : string -> int64
val le32_of_int : int64 -> string

val wrap : ?chan:out_channel -> ?indent:int -> string -> unit
(** Wrap text. *)

val output_spaces : out_channel -> int -> unit
(** Write [n] spaces to [out_channel]. *)

val string_prefix : string -> string -> bool
val string_suffix : string -> string -> bool
val string_find : string -> string -> int
val replace_str : string -> string -> string -> string
val string_nsplit : string -> string -> string list
val string_split : string -> string -> string * string
val string_random8 : unit -> string
val string_lines_split : string -> string list
(** Various string functions. *)

val dropwhile : ('a -> bool) -> 'a list -> 'a list
val takewhile : ('a -> bool) -> 'a list -> 'a list
val filter_map : ('a -> 'b option) -> 'a list -> 'b list
val iteri : (int -> 'a -> 'b) -> 'a list -> unit
val mapi : (int -> 'a -> 'b) -> 'a list -> 'b list
(** Various higher-order functions. *)

val combine3 : 'a list -> 'b list -> 'c list -> ('a * 'b * 'c) list
(** Like {!List.combine} but for triples.  All lists must be the same length. *)

val assoc : ?cmp:('a -> 'a -> int) -> default:'b -> 'a -> ('a * 'b) list -> 'b
(** Like {!List.assoc} but with a user-defined comparison function, and
    instead of raising [Not_found], it returns the [~default] value. *)

val prog : string
(** The program name (derived from {!Sys.executable_name}). *)

val set_quiet : unit -> unit
val quiet : unit -> bool
val set_trace : unit -> unit
val trace : unit -> bool
val set_verbose : unit -> unit
val verbose : unit -> bool
(** Stores the quiet ([--quiet]), trace ([-x]) and verbose ([-v]) flags in a
    global variable. *)

val message : ('a, unit, string, unit) format4 -> 'a
(** Timestamped progress messages.  Used for ordinary messages when
    not [--quiet]. *)

val error : ?exit_code:int -> ('a, unit, string, 'b) format4 -> 'a
(** Standard error function. *)

val warning : ('a, unit, string, unit) format4 -> 'a
(** Standard warning function. *)

val info : ('a, unit, string, unit) format4 -> 'a
(** Standard info function.  Note: Use full sentences for this. *)

val run_main_and_handle_errors : (unit -> unit) -> unit
(** Common function for handling pretty-printing exceptions. *)

val print_version_and_exit : unit -> unit
(** Print the version number and exit.  Implements [--version] flag in
    the OCaml tools. *)

val read_whole_file : string -> string
(** Read in the whole file as a string. *)

val parse_size : string -> int64
(** Parse a size field, eg. [10G] *)

val parse_resize : int64 -> string -> int64
(** Parse a size field, eg. [10G], [+20%] etc.  Used particularly by
    [virt-resize --resize] and [--resize-force] options. *)

val human_size : int64 -> string
(** Converts a size in bytes to a human-readable string. *)

val skip_dashes : string -> string
(** Skip any leading '-' characters when comparing command line args. *)

val compare_command_line_args : string -> string -> int
(** Compare command line arguments for equality, ignoring any leading [-]s. *)

val long_options : (Arg.key * Arg.spec * Arg.doc) list ref
val display_short_options : unit -> 'a
(** Implements [--short-options]. *)
val display_long_options : unit -> 'a
(** Implements [--long-options]. *)

val compare_version : string -> string -> int
(** Compare two version strings. *)

val compare_lvm2_uuids : string -> string -> int
(** Compare two LVM2 UUIDs, ignoring '-' characters. *)

val external_command : string -> string list
(** Run an external command, slurp up the output as a list of lines. *)

val uuidgen : unit -> string
(** Run uuidgen to return a random UUID. *)

val unlink_on_exit : string -> unit
(** Unlink a temporary file on exit. *)

val rmdir_on_exit : string -> unit
(** Remove a temporary directory on exit (using [rm -rf]). *)

val rm_rf_only_files : Guestfs.guestfs -> string -> unit
(** Using the libguestfs API, recursively remove only files from the
    given directory.  Useful for cleaning [/var/cache] etc in sysprep
    without removing the actual directory structure.  Also if [dir] is
    not a directory or doesn't exist, ignore it.

    XXX Could be faster with a specific API for doing this. *)

val truncate_recursive : Guestfs.guestfs -> string -> unit

val detect_file_type : string -> [`GZip | `Tar | `XZ | `Zip | `Unknown]
(** Detect type of a file. *)

val is_block_device : string -> bool
val is_char_device : string -> bool
val is_directory : string -> bool
(** These don't throw exceptions, unlike the [Sys] functions. *)

val absolute_path : string -> string
(** Convert any path to an absolute path. *)

val qemu_input_filename : string -> string
(** Sanitizes a filename for passing it safely to qemu/qemu-img.

    If the filename is something like "file:foo" then qemu-img will
    try to interpret that as "foo" in the file:/// protocol.  To
    avoid that, if the path is relative prefix it with "./" since
    qemu-img won't try to interpret such a path. *)

val mkdir_p : string -> int -> unit
(** Creates a directory, and its parents if missing. *)

val guest_arch_compatible : string -> bool
(** Are guest arch and host_cpu compatible, in terms of being able
    to run commands in the libguestfs appliance? *)