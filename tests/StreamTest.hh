<?hh
namespace Usox\HackTTP;

include_once 'vendor/hhvm/experimental-http-request-response-interfaces/src/StreamInterface.hh';

/**
 * @covers GuzzleHttp\Psr7\Stream
 */
class StreamTest extends \PHPUnit_Framework_TestCase {
	public static $isFReadError = false;

	public function testConstructorInitializesProperties() {
		$handle = fopen('php://temp', 'r+');
		fwrite($handle, 'data');
		$stream = new Stream($handle);

		$meta_data = $stream->getMetadata();

		$this->assertTrue($stream->isReadable());
		$this->assertTrue($stream->isWritable());
		$this->assertTrue($stream->isSeekable());
		$this->assertEquals('php://temp', $meta_data['uri']);
		$this->assertEquals(4, $stream->getSize());
		$this->assertFalse($stream->eof());

		$stream->close();
	}

	public function testConvertsToString() {
		$handle = fopen('php://temp', 'w+');
		fwrite($handle, 'data');
		$stream = new Stream($handle);
		$this->assertEquals('i am deprecated', (string) $stream);
		$this->assertEquals('i am deprecated', (string) $stream);
		$stream->close();
	}

	public function testGetsFullContents() {
		$handle = fopen('php://temp', 'w+');
		fwrite($handle, 'data');
		$stream = new Stream($handle);
		$this->assertEquals('', $stream->getRemainingContents());
		$stream->seek(0);
		$this->assertEquals('data', $stream->getRemainingContents());
		$this->assertEquals('', $stream->getRemainingContents());
		$stream->close();
	}

	public function testGetRemainingContentsReturnsTheCompleteStream() {
		$handle = fopen('php://temp', 'w+');
		fwrite($handle, 'data');
		$stream = new Stream($handle);
		$this->assertEquals('data', (string) $stream->getFullContents());
		$this->assertEquals('data', (string) $stream->getFullContents());
		$stream->close();
	}

	public function testChecksEof() {
		$handle = fopen('php://temp', 'w+');
		fwrite($handle, 'data');
		$stream = new Stream($handle);
		$this->assertSame(4, $stream->tell(), 'Stream cursor already at the end');
		$this->assertFalse($stream->eof(), 'Stream still not eof');
		$this->assertSame('', $stream->read(1), 'Need to read one more byte to reach eof');
		$this->assertTrue($stream->eof());
		$stream->close();
	}

	public function testGetSize() {
		$size = filesize(__FILE__);
		$handle = fopen(__FILE__, 'r');
		$stream = new Stream($handle);
		$this->assertEquals($size, $stream->getSize());
		// Load from cache
		$this->assertEquals($size, $stream->getSize());
		$stream->close();
	}

	public function testEnsuresSizeIsConsistent() {
		$h = fopen('php://temp', 'w+');
		$this->assertEquals(3, fwrite($h, 'foo'));
		$stream = new Stream($h);
		$this->assertEquals(3, $stream->getSize());
		$this->assertEquals(4, $stream->write('test'));
		$this->assertEquals(7, $stream->getSize());
		$this->assertEquals(7, $stream->getSize());
		$stream->close();
	}

	public function testProvidesStreamPosition() {
		$handle = fopen('php://temp', 'w+');
		$stream = new Stream($handle);
		$this->assertEquals(0, $stream->tell());
		$stream->write('foo');
		$this->assertEquals(3, $stream->tell());
		$stream->seek(1);
		$this->assertEquals(1, $stream->tell());
		$this->assertSame(ftell($handle), $stream->tell());
		$stream->close();
	}

	public function testDetachStreamAndClearProperties() {
		$handle = fopen('php://temp', 'r');
		$stream = new Stream($handle);
		$this->assertSame($handle, $stream->detach());
		$this->assertTrue(is_resource($handle), 'Stream is not closed');
		$this->assertNull($stream->detach());

		$this->assertStreamStateAfterClosedOrDetached($stream);

		$stream->close();
	}

	public function testCloseResourceAndClearProperties() {
		$handle = fopen('php://temp', 'r');
		$stream = new Stream($handle);
		$stream->close();

		$this->assertFalse(is_resource($handle));

		$this->assertStreamStateAfterClosedOrDetached($stream);
	}

	private function assertStreamStateAfterClosedOrDetached(Stream $stream) {
		$this->assertFalse($stream->isReadable());
		$this->assertFalse($stream->isWritable());
		$this->assertFalse($stream->isSeekable());
		$this->assertNull($stream->getSize());
		$this->assertEquals(dict[], $stream->getMetadata());

		$throws = function ($fn) {
			try {
				$fn();
			} catch (\Exception $e) {
				$this->assertContains('Stream is detached', $e->getMessage());

				return;
			}

			$this->fail('Exception should be thrown after the stream is detached.');
		};

		$throws(function () use ($stream) { $stream->read(10); });
		$throws(function () use ($stream) { $stream->write('bar'); });
		$throws(function () use ($stream) { $stream->seek(10); });
		$throws(function () use ($stream) { $stream->tell(); });
		$throws(function () use ($stream) { $stream->eof(); });
		$throws(function () use ($stream) { $stream->getRemainingContents(); });
		$throws(function () use ($stream) { $stream->getFullContents(); });
	}

	public function testStreamReadingWithZeroLength() {
		$r = fopen('php://temp', 'r');
		$stream = new Stream($r);

		$this->assertSame('', $stream->read(0));

		$stream->close();
	}

	/**
	 * @expectedException \RuntimeException
	 * @expectedExceptionMessage Length parameter cannot be negative
	 */
	public function testStreamReadingWithNegativeLength() {
		$r = fopen('php://temp', 'r');
		$stream = new Stream($r);

		try {
			$stream->read(-1);
		} catch (\Exception $e) {
			$stream->close();
			throw $e;
		}

		$stream->close();
	}

	/**
	 * @expectedException \RuntimeException
	 * @expectedExceptionMessage Unable to read from stream
	 */
	public function testStreamReadingFreadError() {
		self::$isFReadError = true;
		$r = fopen('php://temp', 'r');
		$stream = new Stream($r);

		try {
			$stream->read(1);
		} catch (\Exception $e) {
			self::$isFReadError = false;
			$stream->close();
			throw $e;
		}

		self::$isFReadError = false;
		$stream->close();
	}
}

function fread($handle, $length) {
	return StreamTest::$isFReadError ? false : \fread($handle, $length);
}
