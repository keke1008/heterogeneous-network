# multihop-arduino

## Testing

```sh
make test
```

## Notice

`./server/`で使用している`tsx`はNode.js 20.0.0で実行するとメモリリークする
[参考](https://github.com/esbuild-kit/tsx/issues/238)
