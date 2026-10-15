# bm_shell - ビットマップ転送型リモートシェルサーバー

<div style="text-align:right">2026年10月15日</div>

## 概要

本ソフトウェアはlinux上で動作するリモートシェルのサーバーです。
SSH [RFC 4250, RFC 4251, RFC 4253, RFC 4254, RFC 4255, RFC 4256]と異なる点は以下のようになります。

1. トランスポートには[espress_stream](http://akino-yo.f5.si/akinoyonika/espress_stream/)を用い、暗号通信を行います。
2. SSHでいうところのシェルセッション相当の機能のみが搭載されています。
3. サーバーからクライアントに送られるデータは文字をレンダリングした結果のビットマップデータとその表示位置となっています。

3の仕様のため、本ソフトウェアのクライアントはフォントデータを一切持つ必要がなく、エスケープシーケンスをパースし、表示のレンダリングを行う必要もありません。
ターミナルがフォントデータ持たなくても漢字や絵文字が制限なく表示できます。
これは、リソースに制限の強い組み込み機器等でターミナルを構成することを想定した仕様です。

## パッケージ

本ソフトウェアはソースパッケージにて提供しています。以下をダウンロードしてください。

- [bm_shell.20261015.tar.gz](http://akino-yo.f5.si/akinoyonika/bm_shell/bm_shell.20261015.tar.gz)

<details><summary>変更履歴</summary>

bm_shell.20261015.tar.gz
: <ul>
    <li>パッケージにLICENSEファイルを追加した。</li>
  </ul>

bm_shell.20261013.tar.gz
: <ul>
    <li>移植性向上のため、テスト用クライアントbm_termの表示部、キー入力部をメインから分離し、モジュール化した。</li>
  </ul>

bm_shell.20260930.tar.gz
: <ul>
    <li>ドキュメントをわずかに修正。</li>
  </ul>

bm_shell.20260925.tar.gz
: <ul>
    <li>初版</li>
  </ul>

</details>

## 導入

### 準備

本ソフトウェアは、基本的なgcc、make等が使えるlinux環境に対し導入できます。
本ソフトウェアの導入、動作の確認はubuntuで行われています。
また、いくつかのライブラリー、コマンドにも依存していますので、以下のようにしてこれらを導入してください。

```
$ sudo apt install build-essential libvterm-dev libfreetype-dev libfontconfig1-dev libsdl2-dev expect
```

さらに、事前に[espress_stream](http://akino-yo.f5.si/akinoyonika/espress_stream/)とそのサンプルアプリケーション<tt>es_gw</tt>と<tt>es_ccat</tt>の導入を行ってください。

最初に適当なディレクトリでパッケージの展開を行っておいてください。

```
$ tar zxvf bm_shell.########.tar.gz
```

### 手順

```
$ cd bm_shell
$ make clean
$ make
$ sudo make uninstall
$ sudo make install
$ make clean
$ cd test/bm_term
$ make clean
$ make
$ sudo make uninstall
$ sudo make install
$ make clean
```

## 使用方法

以下のようにしてサーバーを立ち上げます。

```
$ bm_shell
public key: 7335e63a762123a941a25e5e538dd4d2e47d869280f83af7810e0b3dae8069c9
protocol: ipv4
bind address: (any)
bind port: 12345
close: full
cat command: /usr/local/libexec/bm_shell
```

<tt>public_key</tt>はクライアントが行うサーバー認証のための公開鍵です。
ポート番号はデフォルトで<tt>12345</tt>になっていますので、変更したい場合は<tt>-p</tt>オプションを使用してください。

```
$ bm_shell -p 12345
public key: 7335e63a762123a941a25e5e538dd4d2e47d869280f83af7810e0b3dae8069c9
protocol: ipv4
bind address: (any)
bind port: 12345
close: full
cat command: /usr/local/libexec/bm_shell
```

サーバーは[CTRL] + [C]で終了します。

クライアントは本来、ネットワーク接続する組み込み機器等で動作させることを想定しています。
しかし、テスト用にlinuxのGUI環境で動作するテスト用クライアントが上記導入手順にて導入されています。
これは以下のように立ち上げてください。

```
$ bm_term -p 12345 -k 7335e63a762123a941a25e5e538dd4d2e47d869280f83af7810e0b3dae8069c9
FONT: mono
MODE: 3
LHPX: 16
CWPX: 8
ROWS: 24
COLS: 80
USER: foo
PASS: 
```

<tt>-k 7335...</tt>はサーバーの公開鍵です。サーバー実行時に表示されるものを指定してください。
開始直後に表示される各プロンプトには以下の情報を入力します。

<tt>FONT: </tt>
: フォントを選択する文字列を指定します。
  初期テキストとして<tt>mono</tt>が設定されています。
  日本語環境の場合<tt>noto sans mono, noto color emoji</tt>が良いでしょう。

<tt>MODE: </tt>
: 初期テキストとして<tt>3</tt>が設定されています。
  とりあえずはそのまま入力してください。

<tt>LHPX: </tt>
: 1行の高さをピクセル数で指定します。
  初期テキストとして<tt>16</tt>が設定されています。

<tt>CWPX: </tt>
: 1文字の幅をピクセル数で指定します。
  初期テキストとして<tt>8</tt>が設定されています。

<tt>ROWS: </tt>
: 画面の行数を指定します。
  初期テキストとして<tt>24</tt>が設定されています。

<tt>COLS: </tt>
: 画面の桁数を指定します。
  初期テキストとして<tt>80</tt>が設定されています。

<tt>USER: </tt>
: ログインユーザー名を指定します。

<tt>PASS: </tt>
: ログインパスワードを指定します。
  (エコーバックしません。)

ログインパスワードを入力した後、プロンプトが表示されているウィンドウが生成したら成功です。
ウィンドウにフォーカスを移せば、キー入力が可能になり、シェルを操作できるかと思います。
シェルで<tt>exit</tt>を入力すればウィンドウが閉じ、クライアントが終了します。

もし、他のマシンからネットワークを通じてサーバーに接続したい場合は以下のようにしてください。

```
$ bm_term -a foo -p 12345 -k 7335e63a762123a941a25e5e538dd4d2e47d869280f83af7810e0b3dae8069c9
FONT: mono
MODE: 3
LHPX: 16
CWPX: 8
ROWS: 24
COLS: 80
USER: foo
PASS: 
```

<tt>-a foo</tt>はサーバーマシンのドメイン指定です(<tt>###.###.###.###</tt>形式でIPアドレス指定可)。

前述のとおり、bm_shellは豊富なフォントデータを取り扱うのが難しい組み込み機器等で動作するクライアントのためにデザインされています。
linuxのGUI環境で動作するbm_termは実用的ではありません。
(SSHクライアントを使用することに対するメリットがほとんどありません。)
このため、linux用bm_termはテスト用としています。

## ユーセージ

### <tt>bm_shell [-4 | -6] [-a \<bind address\>] [-p \<bind port\>]</tt>

<tt>-4</tt>
: プロトコルにipv4を使用する。 (デフォルト)

<tt>-6</tt>
: プロトコルにipv6を使用する。 (非デフォルト)

<tt>-a \<bind address\></tt>
: 結合するアドレス("<tt>nnn.nnn.nnn</tt>" or "<tt>xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx%ifname</tt>")を指定する。 (デフォルト: 任意)

<tt>-p \<bind port\></tt>
: 結合するポート番号を指定する。 (デフォルト値: "<tt>12345</tt>")

### <tt>bm_term [-4 | -6] [-a \<destination address\> [-p \<destination port\>] [-b \<bind address\>] [-q \<bind port\>]</tt>

<tt>-4</tt>
: プロトコルにipv4を使用する。 (デフォルト)

<tt>-6</tt>
: プロトコルにipv6を使用する。 (非デフォルト)

<tt>-a \<destination address\></tt>
: 接続先のドメイン名、アドレス("<tt>nnn.nnn.nnn</tt>" or "<tt>xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx%ifname</tt>")を指定する。 (デフォルト: "<tt>localhost</tt>")

<tt>-p \<destination port\></tt>
: 接続先のポート番号を指定する。 (デフォルト値: "<tt>12345</tt>")

<tt>-b \<bind address\></tt>
: 結合するアドレス("<tt>nnn.nnn.nnn</tt>" or "<tt>xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx</tt>")を指定する。 (デフォルト: 任意)

<tt>-q \<bind port\></tt>
: 結合するポート番号を指定する。 (デフォルト: 任意)

## ライセンス

MITライセンスに基づいてリリースされています。

<details><summary>Copyright (c) 2026 akinoyonika</summary>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</details>
