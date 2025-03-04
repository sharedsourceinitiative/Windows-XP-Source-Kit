package File::Spec::Functions;

use File::Spec;
use strict;

use vars qw(@ISA @EXPORT @EXPORT_OK);

require Exporter;

@ISA = qw(Exporter);

@EXPORT = qw(
	canonpath
	catdir
	catfile
	curdir
	rootdir
	updir
	no_upwards
	file_name_is_absolute
	path
);

@EXPORT_OK = qw(
	devnull
	tmpdir
	splitpath
	splitdir
	catpath
	abs2rel
	rel2abs
);

foreach my $meth (@EXPORT, @EXPORT_OK) {
    my $sub = File::Spec->can($meth);
    no strict 'refs';
    *{$meth} = sub {&$sub('File::Spec', @_)};
}


1;
__END__

=head1 NAME

File::Spec::Functions - portably perform operations on file names

=head1 SYNOPSIS

	use File::Spec::Functions;
	$x = catfile('a','b');

=head1 DESCRIPTION

This module exports convenience functions for all of the class methods
provided by File::Spec.

For a reference of available functions, please consult L<File::Spec::Unix>,
which contains the entire set, and which is inherited by the modules for
other platforms. For further information, please see L<File::Spec::Mac>,
L<File::Spec::OS2>, L<File::Spec::Win32>, or L<File::Spec::VMS>.

=head2 Exports

The following functions are exported by default.

	canonpath
	catdir
	catfile
	curdir
	rootdir
	updir
	no_upwards
	file_name_is_absolute
	path


The following functions are exported only by request.

	devnull
	tmpdir
	splitpath
	splitdir
	catpath
	abs2rel
	rel2abs

=head1 SEE ALSO

File::Spec, File::Spec::Unix, File::Spec::Mac, File::Spec::OS2,
File::Spec::Win32, File::Spec::VMS, ExtUtils::MakeMaker
