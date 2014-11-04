use AnyEvent::WebSocket::Client 0.12;
 
my $client = AnyEvent::WebSocket::Client->new;
 
$client->connect("ws://localhost:1234/service")->cb(sub {
  my $connection = eval { shift->recv };
  if($@) {
    # handle error...
  }
   
  # send a message through the websocket...
  $connection->send('a message');
   
  # recieve message from the websocket...
  $connection->on(each_message => sub {
    # $connection is the same connection object
    # $message isa AnyEvent::WebSocket::Message
    my($connection, $message) = @_;
    ...
  });
   
  # handle a closed connection...
  $connection->on(finish => sub {
    # $connection is the same connection object
    my($connection) = @_;
    ...
  });
 
  # close the connection (either inside or
  # outside another callback)
  $connection->close;
 
});