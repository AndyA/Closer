#!/usr/bin/env perl

use strict;
use warnings;

use List::Util qw( first max );

use Data::Dumper;

my ( $dis, $hex ) = ( shift, shift );
die "Usage: hexmix.pl <dis> <hex> > <mixed>\n"
 if @ARGV || !defined $hex;

my $data = read_hex( $hex );
hexmix( $dis, $data );

sub hexmix {
  my ( $name, $data ) = @_;

  my @lines = ();

  open my $fh, '<', $name or die "Can't read $name: $!\n";
  while ( defined( my $line = <$fh> ) ) {
    chomp $line;
    if ( $line =~ /^([0-9a-f]{8,})\s*(.*)/ ) {
      push @lines, [ hex( substr $1, -8 ), $1, '', $2 ];
    }
    else {
      push @lines, [ undef, $line, '' ];
    }
  }

  my @bytes = @{ $data->{bytes} };

  for my $ln ( 0 .. $#lines ) {
    if ( defined $lines[$ln][0] ) {
      my $addr = $lines[$ln][0];
      my $next_addr = first { defined $_ }
      map { $_->[0] } @lines[ $ln + 1 .. $#lines ];
      $lines[$ln][2] = hexdump(
        defined $next_addr
        ? ( splice @bytes, 0, $next_addr - $addr )
        : ( splice @bytes )
      );
    }
  }

  my $width = max( map { length $_->[2] } @lines );
  my $fmt = "%s %-${width}s %s\n";
  for my $ln ( @lines ) {
    if ( defined $ln->[0] ) {
      printf $fmt, @{$ln}[ 1 .. 3 ];
    }
    else {
      printf "%s\n", $ln->[1];
    }
  }
}

sub hexdump {
  join ' ', map { sprintf '%02x', $_ } @_;
}

sub read_hex {
  my $name = shift;

  my $next_loc = undef;
  my $data     = {};
  my @bytes    = ();

  open my $fh, '<', $name or die "Can't read $name: $!\n";
  while ( defined( my $line = <$fh> ) ) {
    chomp $line;
    next unless $line =~ /^ ( [0-9a-f] {8,} )
                            ( (?: \s [0-9a-f] {2} ) {1,16} ) /xi;
    my ( $addr, $hex ) = ( $1, $2 );
    $hex =~ s/^\s+//;
    my $loc = hex substr $addr, -8;
    if ( defined $next_loc ) {
      die sprintf "Address mismatch: expected %x, got %x\n",
       $next_loc, $loc
       unless $next_loc == $loc;
    }
    else {
      $data->{base} = $loc;
    }
    $next_loc = $loc + 16;
    push @bytes, map hex, split /\s+/, $hex;
  }
  $data->{bytes} = \@bytes;
  return $data;
}

# vim:ts=2:sw=2:sts=2:et:ft=perl

