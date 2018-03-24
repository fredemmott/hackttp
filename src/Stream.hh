<?hh // strict

namespace Usox\HackTTP;

use Facebook\Experimental\Http\Message\StreamInterface;
use HH\Lib\C;

final class Stream implements StreamInterface {

	private ?int $size;

	private bool $has_uri;

	private bool $readable = false;

	private bool $writable = false;

	private bool $seekable = false;

	private static vec<string> $read_modes = vec[
		'r', 'w+', 'r+', 'x+', 'c+', 'rb', 'w+b', 'r+b', 'x+b',
		'c+b', 'rt', 'w+t', 'r+t', 'x+t', 'c+t', 'a+'
	];

	private static vec<string> $write_modes = vec[
		'w', 'w+', 'rw', 'r+', 'x+', 'c+', 'wb', 'w+b', 'r+b',
		'x+b', 'c+b', 'w+t', 'r+t', 'x+t', 'c+t', 'a', 'a+'
	];

	public function __construct(private ?resource $stream) {
		if ($stream === null) {
			throw new \InvalidArgumentException('Stream must be a resource');
		}

		$meta = stream_get_meta_data($this->stream);
		$this->seekable = (bool) $meta['seekable'];
		$this->readable = C\contains(static::$read_modes, $meta['mode']);
		$this->writable = C\contains(static::$write_modes, $meta['mode']);
		$this->has_uri = $meta['uri'] !== null;
	}

	public function __destruct(): void {
		$this->close();
	}

	public function getContents(): string {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}

		$contents = stream_get_contents($this->stream);

		if ($contents === false) {
			throw new \RuntimeException('Unable to read stream contents');
		}

		return $contents;
	}

	public function close(): void {
		if ($this->stream !== null) {
			fclose($this->stream);
			$this->detach();
		}
	}

	public function detach(): ?resource {
		if ($this->stream === null) {
			return null;
		}

		$result = $this->stream;
		$this->stream = $this->size = null;
		$this->has_uri = $this->readable = $this->writable = $this->seekable = false;

		return $result;
	}

	public function getSize(): ?int {
		if ($this->size !== null) {
			return $this->size;
		}

		if ($this->stream === null) {
			return null;
		}

		if ($this->has_uri === true) {
			clearstatcache();
		}

		$stats = fstat($this->stream);

		if (array_key_exists('size', $stats)) {
			$this->size = $stats['size'];
			return $this->size;
		}

		return null;
	}

	public function isReadable(): bool {
		return $this->readable;
	}

	public function isWritable(): bool {
		return $this->writable;
	}

	public function isSeekable(): bool {
		return $this->seekable;
	}

	public function eof(): bool {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}

		return feof($this->stream);
	}

	public function tell(): int {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}

		$result = ftell($this->stream);

		if ($result === false) {
			throw new \RuntimeException('Unable to determine stream position');
		}

		return $result;
	}

	public function rewind(): void {
		$this->seek(0);
	}

	public function seek(int $offset, int $whence = \SEEK_SET): void {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}
		if ($this->isSeekable() === false) {
			throw new \RuntimeException('Stream is not seekable');
		}
		if (fseek($this->stream, $offset, $whence) === -1) {
			throw new \RuntimeException('Unable to seek to stream position '
				. $offset . ' with whence ' . var_export($whence, true));
		}
	}

	public function read(int $length): string {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}
		if ($this->isReadable() === false) {
			throw new \RuntimeException('Cannot read from non-readable stream');
		}
		if ($length < 0) {
			throw new \RuntimeException('Length parameter cannot be negative');
		}

		if ($length === 0) {
			return '';
		}

		$content = fread($this->stream, $length);
		if ($content === false) {
			throw new \RuntimeException('Unable to read from stream');
		}

		return $content;
	}

	public function write(string $string): int {
		if ($this->stream === null) {
			throw new \RuntimeException('Stream is detached');
		}
		if ($this->isWritable() === false) {
			throw new \RuntimeException('Cannot write to a non-writable stream');
		}

		$this->size = null;
		$bytes = fwrite($this->stream, $string);

		if ($bytes === false) {
			throw new \RuntimeException('Unable to write to stream');
		}

		return $bytes;
	}

	public function getMetadata(): dict<string, mixed> {
		if ($this->stream === null) {
			return dict[];
		}
		return dict(stream_get_meta_data($this->stream));
	}

	public function __toString(): string {
		return 'i am deprecated';
	}
}