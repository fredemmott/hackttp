<?hh // strict

namespace Usox\HackTTP;

use namespace Facebook\Experimental\Http\Message;
use namespace HH\Lib\{Experimental\IO, Experimental\Filesystem, Str};

final class UploadedFile implements Message\UploadedFileInterface {

  private ?string $file;

  private bool $moved = false;

  public function __construct(
    private IO\ReadHandle $stream,
    private ?int $size,
    private ?Message\UploadedFileError $upload_status = null,
    private string $client_filename = '',
    private string $client_media_type = '',
  ) {
  }

  private function validateActive(): void {
    if ($this->upload_status !== null) {
      throw new \RuntimeException('Cannot retrieve stream due to upload error');
    }
    if ($this->moved) {
      throw new \RuntimeException(
        'Cannot retrieve stream after it has already been moved',
      );
    }
  }

  public function moveTo(string $target_path): void {
    $this->validateActive();
    if (Str\length($target_path) === 0) {
      throw new \InvalidArgumentException(
        'Invalid path provided for move operation; must be a non-empty string',
      );
    }
    \HH\Asio\join($this->write($target_path));

    $this->moved = true;
  }

  private async function write(string $target_path): Awaitable<void> {
    await using $target = Filesystem\open_write_only($target_path);
    await $target->writeAsync($this->stream->rawReadBlocking());
    await $this->stream->closeAsync();
  }

  public function getSize(): ?int {
    return $this->size;
  }

  public function getError(): ?Message\UploadedFileError {
    return $this->upload_status;
  }

  public function getClientFilename(): string {
    return $this->client_filename;
  }

  public function getClientMediaType(): string {
    return $this->client_media_type;
  }

  public function getStream(): IO\ReadHandle {
    return $this->stream;
  }
}
