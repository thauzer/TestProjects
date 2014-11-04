use Net::WebSocket::Server;
 
my $origin = 'http://example123.com';
 
Net::WebSocket::Server->new(
    listen => 9000,
    on_connect => sub {
        print "connect\n";
        my ($serv, $conn) = @_;
        $conn->on(
            handshake => sub {
                print "handshake\n";
                my ($conn, $handshake) = @_;
                # if ($handshake->req->origin ne $origin) {
                    # print "not equal origin\n";
                    # print "My origin: ".$origin."\n";
                    # print "Received origin: ".$handshake->req->origin."\n";
                    # $conn->disconnect();
                # }
                #$conn->disconnect() unless $handshake->req->origin eq $origin;
            },
            utf8 => sub {
                my ($conn, $msg) = @_;
                print "utf8 message: ".$msg."\n";
                $_->send_utf8($msg) for $conn->server->connections;
            },
            binary => sub {
                my ($conn, $msg) = @_;
                print "binary message: ".$msg."\n";
                $_->send_binary($msg) for $conn->server->connections;
            },
        );
    },
)->start;