#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include <memory>
using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  return _bytes_in_flight;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "unimplemented consecutive_retransmissions() called" );
  return total_retransmission_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "unimplemented push() called" );
  while ( ( _windos_size == 0 ? 1 : _windos_size ) > _bytes_in_flight ) {
    if ( FIN_sent_ ) {
      break; // Is finished.
    }

    auto msg { make_empty_message() };
    if ( not SYN_sent_ ) {
      msg.SYN = true;
      SYN_sent_ = true;
    }

    const uint64_t remaining { ( _windos_size == 0 ? 1 : _windos_size ) - _bytes_in_flight };
    const size_t len { min( TCPConfig::MAX_PAYLOAD_SIZE, remaining - msg.sequence_length() ) };
    auto&& payload { msg.payload };
    while ( reader().bytes_buffered() != 0U and payload.size() < len ) {
      string_view view { reader().peek() };
      view = view.substr( 0, len - payload.size() );
      payload += view;
      input_.reader().pop( view.size() );
    }

    if ( not FIN_sent_ and remaining > msg.sequence_length() and reader().is_finished() ) {
      msg.FIN = true;
      FIN_sent_ = true;
    }

    if ( msg.sequence_length() == 0 ) {
      break;
    }

    transmit( msg );
    if ( !timer_.is_active() ) {
      timer_.start();
    }
    next_abs_seqno_ += msg.sequence_length();
    _bytes_in_flight += msg.sequence_length();
    outstanding_message_.emplace( move( msg ) );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap( next_abs_seqno_, isn_ ), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( input_.has_error() ) {
    return;
  }
  if ( msg.RST ) {
    input_.set_error();
    return;
  }

  _windos_size= msg.window_size;
  if ( not msg.ackno.has_value() ) {
    return;
  }
  const uint64_t recv_ack_abs_seqno { msg.ackno->unwrap( isn_, next_abs_seqno_ ) };
  if ( recv_ack_abs_seqno > next_abs_seqno_ ) {
    return;
  }
  bool has_acknowledgment { false };
  while ( not outstanding_message_.empty() ) {
    const auto& message { outstanding_message_.front() };
    if ( ack_abs_seqno_ + message.sequence_length() > recv_ack_abs_seqno ) {
      break; // Must be fully acknowledged by the TCP receiver.
    }
    has_acknowledgment = true;
    ack_abs_seqno_ += message.sequence_length();
    _bytes_in_flight -= message.sequence_length();
    outstanding_message_.pop();
  }
  if ( has_acknowledgment ) {
    total_retransmission_ = 0;
    timer_.reload( initial_RTO_ms_ );
    outstanding_message_.empty() ? timer_.stop() : timer_.start();
  }
}
void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( timer_.tick( ms_since_last_tick ).is_expired() ) {
    if ( outstanding_message_.empty() ) {
      return;
    }
    transmit( outstanding_message_.front() );
    if ( _windos_size != 0 ) {
      total_retransmission_ += 1;
      timer_.exponential_backoff();
    }
    timer_.reset();
  }
}
