/**
 * UDP reliability.
 */

#pragma once

#include <cstdint>

// Step 1: Initial connection scenario
// Client sends a packet with type <connect>
// Session id is 0, indicating that connection is not established yet.
// Server receives the request, responds with <handshake_kex> to initiate
// the asymmetric key exchange.
// Client and server performs the key exchange and agrees upon using secret key S.
// Server assigns a <session id> to the connection and persists the session state (key S, session id)
// in the session map.
// Client uses the associated session id from now on, and encrypts all messages with the key S
// from now on.
// Key derivation function?

// Step 2: Authentication
// If the user-space code requires authentication, the user has to implement it themselves.
// Client sends user-space data packet, requesting login
// Server retrieves the associated connection with session id.
// Server checks whether the session id is already authenticated. If so, server rejects the authentication attempt.
// If the connection is not authenticated, server checks the provided credentials. If they match, server marks
// the connection as "authenticated" and responds with auth_ok data packet. Then, user code calls the switch_key function
// to change the connection's symmetric encryption key. Server sends the new encryption key to the client, and client
// is expected to encrypt/decrypt the new packets with this key from now on. Only the authenticated client knows the
// right key for the session, so only the authenticated user can control the connection.

// TODO: Continue from here. 
// TODO: Reconsider QUIC?

// Server checks the credentials, 

// timeout:

// where to store the state?
// <session id>: 64 bit, sequential?
//

// account id.
// switch key

namespace mt { namespace nexus {

    enum class e_arq_msg_type : std::uint8_t
    {
        connect,
        handshake_kex,
        handshake_kex_ok,
        switch_key, // reliable
        disconnect,
        data_reliable,
        data_unreliable,
        ack /* Explicit ACK */
    };

    // Configuration option
    // MTU size.
    // Aim for 1200 bytes. Can be adaptive?
    // TODO: Maybe discover path mtu via setting dont fragment bit?
    // IMPORTANT: Avoid IP fragmentation at all costs.

    // 

    /**
     * All data is little-endian unless specified otherwise.
     */

     // source ip-port is the session id.
    // require either authentication or encrypted message.


    // Unauthenticated client

     // 16 bytes?
    struct arq_header {

        struct {
            e_arq_msg_type type : 3;
            std::uint8_t eom : 1;
            std::uint8_t compressed : 1;
            std::uint8_t unused : 3;
        } flags;

        /**
         * 65535 clients should be good enough for now...
         * An unauthenticated client should have this set to 0.
         */
        std::uint16_t session_id;


        /**
         * Which channel does this packet belong to?
         * Determined by the application.
         * Higher channel ID's have higher priority.
         */
        std::uint8_t channel_id;

        /**
         * Unique sequence number of the packet.
         */
        std::uint8_t sequence_number;

        /**
         * Determines the datagram ID associated with the packet.
         * Used for fragmented datagram reassembly and reordering
         * purposes. The index is automatically rotated.
         */
        std::uint8_t message_number;

        std::uint16_t message_length;

        /**
         * Offset of the message fragment.
         */
        std::uint16_t message_offset;

        struct {
            /**
             * Base number for selective acknowledgement.
             */
            std::uint8_t base;
            /**
             * Bitmap for the SACK. Base + bit position represents
             * the corresponding sequence number's acknowledgement
             * status.
             */
            std::uint8_t bitmap;
        } sack;

        /**
         * Amount of empty space in receive window.
         * Based on datagrams, not the size in bytes.
         */
        std::uint16_t window_size;
    };

    /**
        Key concepts:

        // multiple channels

        Sliding window:

        Sequence number of packets that sender and receiver is allowed
        to accept without waiting for an ACK.

        Selective repeat ARQ:
        Sender: Retransmit only specific packets that were not acknowledged
        Receiver: Buffer out-of order packets.

        Sender and receiver must have send and receive queues.
        Sender and receiver must track sequence numbers.

        ACK mechanisms:
        - Cumulative ACK
        - Selective ACK

        Retransmission:
        - Timer-based
        - When NAK received

        Selective ACK bitmap:
        - Base number + bitmap
        - 100 + uint8, each bit represents next number, e.g.: 101,102,103,104,105,106,107,108
        - bit is set to 1 if received, else 0

        The code must handle the fragmentation itself.

     */

    /*
     * Reliable, ordered
     * Reliable, unordered
     * Unreliable, unordered
     * Unreliable, unordered, discard unordered
     */

    /**
     * Automatic repeat request (ARQ) implementation
     */
    class arq {

        // TODO: Needs a storage for packets in transmission
        // TODO: Needs a retransmit timer
        //
    };
}} // namespace mt::nexus